#pragma once

#include "GLib/Win/ComPtr.h"

#include <atomic>

#define GLIB_COM_RULE_OF_FIVE(ClassName)\
public:\
	ClassName() = default;\
	ClassName(const ClassName& other) = delete;\
	ClassName(ClassName&& other) noexcept = delete;\
	ClassName& operator=(const ClassName& other) = delete;\
	ClassName& operator=(ClassName&& other) noexcept = delete;\
protected:\
	virtual ~ClassName() = default;\

namespace GLib
{
	namespace Win
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

			template <typename T, typename Last>
			HRESULT Qi(T * t, const IID & iid, void** ppvObject)
			{
				if (iid == __uuidof(Last))
				{
					auto * i = static_cast<Last*>(t);
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
					auto * i = static_cast<First*>(t);
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
			typedef I1 DefaultInterface;
			typedef ComPtr<I1> PtrType;

			GLIB_COM_RULE_OF_FIVE(Unknown)

			ULONG ReferenceCount() const
			{
				return ref;
			}

			virtual HRESULT STDMETHODCALLTYPE QueryInterface(const IID& id, void** ppvObject) override
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
	}
}