#pragma once

#include "GLib/Win/ComPtr.h"

#include <atomic>

#define WRAP(x) x // NOLINT(cppcoreguidelines-macro-usage)
#define GLIB_COM_RULE_OF_FIVE(ClassName) /* NOLINT(cppcoreguidelines-macro-usage)*/\
public:\
	(ClassName)() = default;\
	(ClassName)(const WRAP(ClassName) & other) = delete;\
	(ClassName)(WRAP(ClassName) && other) noexcept = delete;\
	WRAP(ClassName) & operator=(const WRAP(ClassName) & other) = delete;\
	WRAP(ClassName) & operator=(WRAP(ClassName) && other) noexcept = delete;\
protected:\
	virtual ~ClassName() = default;\

namespace GLib::Win
{
	template <typename T> class ComPtrBase;
	template <typename T> class ComPtr;

	namespace Detail
	{
		template <typename Last>
		std::string type_name()
		{
			return std::string(typeid(Last).name());
		}

		// specialise this to avoid ambiguous casts to base interfaces, move to another namespace?
		template <typename I, typename T> I * Cast(T * t) { return static_cast<I*>(t); }

		template <typename T, typename Last>
		HRESULT Qi(T * t, const IID & iid, void** ppvObject)
		{
			if (iid == __uuidof(Last))
			{
				auto * i = Cast<Last>(t);
				i->AddRef();
				*ppvObject = i;
				return S_OK;
			}
#ifdef SIMPLECOM_LOG_QI_MISS
			ComDiags::LogIid("QI miss", typeid(t).name(), iid);
#else
			UNREFERENCED_PARAMETER(iid);
#endif
			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}

		template <typename T, typename First, typename Second, typename... Rest>
		HRESULT Qi(T * t, const IID & iid, void** ppvObject)
		{
			if (iid == __uuidof(First)) // just call above?
			{
				auto* i = Cast<First>(t);
				i->AddRef();
				*ppvObject = i;
				return S_OK;
			}
			return Qi<T, Second, Rest...>(t, iid, ppvObject);
		}
	}

	template <typename I1, typename... I2> // i1,i2 :: IUnknown
	class __declspec(novtable) Unknown : public I1, public I2...
	{
		template <typename T> friend class ComPtrBase; // allows ComPtr to hold concrete class

		std::atomic<ULONG> ref = 1;

	public:
		using DefaultInterface = I1;
		using PtrType = ComPtr<I1>;

		GLIB_COM_RULE_OF_FIVE(Unknown)

		ULONG ReferenceCount() const
		{
			return ref;
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(const IID& id, void** ppvObject) override
		{
			if (!ppvObject)
			{
				return E_POINTER;
			}
			if (id == __uuidof(IUnknown))
			{
				auto i = static_cast<IUnknown*>(static_cast<I1*>(this));
				i->AddRef();
				*ppvObject = i;
				return S_OK;
			}
			return Detail::Qi<Unknown, I1,  I2...>(this, id, ppvObject);
		}

		ULONG STDMETHODCALLTYPE AddRef() override
		{
			return ++ref;
		}

		ULONG STDMETHODCALLTYPE Release() override
		{
			const ULONG ret = --ref;
			if (ret == 0)
			{
				delete this;
			}
			return ret;
		}
	};

	// Unknown2, takes two type lists, implemented interfaces and QI interfaces
	// TODO: Unknown3 generates implemented interfaces from QI interfaces by static base class filtering
	template <typename...> struct TypeList {};
	template<typename T, typename TypeListOne, typename TypeListTwo> class Unknown2;

	template<typename T, typename... TopLevelInterfaces, typename... AllInterfaces>
	class __declspec(novtable) Unknown2<T, TypeList<TopLevelInterfaces...>, TypeList<AllInterfaces...>>
		: public TopLevelInterfaces...
	{
		template <typename T> friend class ComPtrBase; // allows ComPtr to hold concrete class

		std::atomic<ULONG> ref = 1;

	public:
		using DefaultInterface = typename std::tuple_element<0, std::tuple<TopLevelInterfaces...>>::type;
		using PtrType = ComPtr<DefaultInterface>;

		GLIB_COM_RULE_OF_FIVE(Unknown2)

		ULONG ReferenceCount() const
		{
			return ref;
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(const IID& id, void** ppvObject) override
		{
			if (!ppvObject)
			{
				return E_POINTER;
			}
			if (id == __uuidof(IUnknown))
			{
				auto i = static_cast<IUnknown*>(static_cast<DefaultInterface*>(this));
				i->AddRef();
				*ppvObject = i;
				return S_OK;
			}
			return Detail::Qi<T, AllInterfaces...>(static_cast<T*>(this), id, ppvObject);
		}

		ULONG STDMETHODCALLTYPE AddRef() override
		{
			return ++ref;
		}

		ULONG STDMETHODCALLTYPE Release() override
		{
			const ULONG ret = --ref;
			if (ret == 0)
			{
				delete this;
			}
			return ret;
		}
	};
}