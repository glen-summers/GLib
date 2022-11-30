#pragma once

#include <GLib/Win/ComPtr.h>

#ifdef COM_LOG_QI_MISS
#include <GLib/Win/DebugWrite.h>
#include <GLib/Win/Registry.h>
#include <GLib/Win/Uuid.h>
#endif

#include <GLib/TypePredicates.h>

#include <atomic>

namespace GLib::Win
{
	template <typename T>
	class ComPtrBase;

	template <typename T>
	class ComPtr;

	namespace Detail
	{
		// specialise this to avoid ambiguous casts to base interfaces
		template <typename I, typename T>
		I * Cast(T * t)
		{
			return static_cast<I *>(t);
		}

		template <typename F, typename... R>
		struct First
		{
			using Value = F;
		};

		template <typename T, typename Last>
		HRESULT Qi(T * t, const IID & iid, void ** ppvObject)
		{
			if (iid == GetUuId<Last>())
			{
				auto * i = Cast<Last>(t);
				i->AddRef();
				*ppvObject = i;
				return S_OK;
			}
#ifdef COM_LOG_QI_MISS
			std::string itf = "Interface\\" + ToString(Util::Uuid(iid)), name;
			if (RegistryKeys::ClassesRoot.KeyExists(itf))
			{
				name = RegistryKeys::ClassesRoot.OpenSubKey(itf).GetString("");
			}
			Debug::Write("QI miss {0} : {1} {2}", typeid(T).name(), itf, name);
#else
			static_cast<void>(iid);
#endif
			*ppvObject = nullptr;
			return E_NOINTERFACE;
		}

		template <typename T, typename First, typename Second, typename... Rest>
		HRESULT Qi(T * t, const IID & iid, void ** ppvObject)
		{
			if (iid == GetUuId<First>()) // just call above?
			{
				auto * i = Cast<First>(t);
				i->AddRef();
				*ppvObject = i;
				return S_OK;
			}
			return Qi<T, Second, Rest...>(t, iid, ppvObject);
		}

		template <typename Interfaces>
		struct __declspec(novtable) Implements;

		template <typename... Interfaces>
		struct __declspec(novtable) Implements<GLib::Util::Tuple<Interfaces...>> : Interfaces...
		{};
	}

	template <typename T, typename... Interfaces>
	class Unknown : public Detail::Implements<typename GLib::Util::SelfTypeFilter<TypePredicates::HasNoInheritor, Interfaces...>::TupleType::Type>
	{
		std::atomic<ULONG> ref = 1;

	public:
		using DefaultInterface = typename Detail::First<Interfaces...>::Value;
		using PtrType = ComPtr<DefaultInterface>;

		Unknown() = default;
		Unknown(Unknown const &) = delete;
		Unknown(Unknown &&) = delete;
		Unknown & operator=(Unknown const &) = delete;
		Unknown & operator=(Unknown &&) = delete;

	protected:
		virtual ~Unknown() = default;

		HRESULT STDMETHODCALLTYPE QueryInterface(IID const & id, void ** ppvObject) override
		{
			if (ppvObject == nullptr)
			{
				return E_POINTER;
			}
			if (id == GetUuId<IUnknown>())
			{
				auto * i = static_cast<IUnknown *>(static_cast<DefaultInterface *>(this));
				i->AddRef();
				*ppvObject = i;
				return S_OK;
			}
			return Detail::Qi<T, Interfaces...>(static_cast<T *>(this), id, ppvObject);
		}

		ULONG STDMETHODCALLTYPE AddRef() override
		{
			return ++ref;
		}

		ULONG STDMETHODCALLTYPE Release() override
		{
			ULONG const ret = --ref;
			if (ret == 0)
			{
				delete this;
			}
			return ret;
		}
	};
}
