#pragma once

#include "GLib/Win/WinException.h"

#define COM_PTR_DEBUG // testing

#ifdef COM_PTR_DEBUG
#include "GLib/Win/DebugStream.h"
#endif

#include <objbase.h>
#include <type_traits>

#include "cassert"

namespace GLib::Win
{
	namespace Detail
	{
		template <typename T>
		class Restricted : public T
		{
		public:
			Restricted() = delete;
			Restricted(const Restricted &) = delete;
			Restricted(Restricted &&) = delete;
			Restricted & operator = (const Restricted &) = delete;
			Restricted & operator = (Restricted &&) = delete;
			~Restricted() = delete;

		private:
			ULONG STDMETHODCALLTYPE AddRef();
			ULONG STDMETHODCALLTYPE Release();
		};

		template <typename T, typename enable = void>
		struct CanRestrict
		{
			using Type = Restricted<T>;
		};

		template <typename T>
		struct CanRestrict<T, typename std::enable_if<std::is_final<T>::value>::type>
		{
			using Type = T;
		};
	}
#define COM_PTR_FWD(x) template <> struct Util::Detail::CanRestrict<x> { using Type = x; }; // NOLINT(cppcoreguidelines-macro-usage) just remove this?

	// avoid base
	// rewrite and simplify
	template <typename T>
	class ComPtrBase
	{
		T * p = nullptr;

	public:
		ComPtrBase & operator = (const ComPtrBase &) = delete;

	protected:
		ComPtrBase() = default;

		ComPtrBase(const ComPtrBase & other)
			: p(other.p)
		{
			InternalAssign(other.p);
		}

		ComPtrBase(ComPtrBase && right) noexcept
			: p(right.p)
		{
			right.p = nullptr;
		}

		~ComPtrBase() = default;

		template<typename U>
		explicit ComPtrBase(ComPtrBase<U> && right)
			: p(right.p)
		{
			right.p = nullptr;
		}

		ComPtrBase & operator=(ComPtrBase && right) noexcept
		{
			Swap(std::move(right));
			return *this;
		}

		T * Get() const noexcept
		{
			return p;
		}

		// avoid
		T * const * GetAddress() const noexcept
		{
			return &p;
		}

		void InternalAssign(T * other) noexcept
		{
			p = other;
			if (p)
			{
				p->AddRef();
			}
		}

		template<typename U>
		void InternalAssign(const ComPtrBase<U>& other)
		{
			InternalAssign(other.p);
		}

		void InternalAttach(T * other) noexcept
		{
			p = other;
		}

		T * InternalDetach()
		{
			T * detached = p;
			p = nullptr;
			return detached;
		}

		void InternalRelease() const noexcept
		{
			// as this is currently only called from a destructor it cannot get the recursive Release call
			// mentioned at https://msdn.microsoft.com/magazine/dn904668.aspx
			if (p != nullptr)
			{
#ifndef COM_PTR_DEBUG
				p->Release();
#else
				if (p->Release() == 0)
				{
					GLib::Win::Debug::Stream() << typeid(p).name() << " deleted" << std::endl;;
				}
#endif
			}
		}

		void Swap(ComPtrBase & right) noexcept
		{
			std::swap(p, right.p);
		}

		template<typename U> friend class ComPtrBase;

	public:
		// diagnostics
		unsigned int UseCount() const
		{
			if (p == nullptr)
			{
				return 0;
			}
			p->AddRef();
			return p->Release();
		}
	};

	template <typename T>
	class ComPtr : public ComPtrBase<T>
	{
	public:
		using BaseType = ComPtrBase<T>;

		ComPtr() noexcept = default;

		// implicit ptr ctor allows conversions, differs from shared_ptr definition, should be ok as non-com pointers will cause compile errors??
		template <typename U>
		explicit ComPtr(U * u)
		{
			BaseType::InternalAssign(u);
		}

		explicit ComPtr(std::nullptr_t) noexcept
		{}

		ComPtr(const ComPtr & other) noexcept
		{
			BaseType::InternalAssign(other);
		}

		template<typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
		explicit ComPtr(const ComPtr<U> & other) noexcept
		{
			BaseType::InternalAssign(other);
		}

		ComPtr(ComPtr && right) noexcept
			: BaseType(std::move(right))
		{}

		template<typename U, typename = typename std::enable_if<std::is_convertible<U*, T*>::value, void>::type>
		explicit ComPtr(ComPtr<U> && right) noexcept
			: BaseType(std::move(right))
		{}

		ComPtr & operator=(ComPtr && right) noexcept
		{
			ComPtr(std::move(right)).Swap(*this);
			return *this;
		}

		template<typename U>
		ComPtr & operator=(ComPtr<U>&& right) noexcept
		{
			ComPtr(std::move(right)).Swap(*this);
			return *this;
		}

		~ComPtr() noexcept
		{
			BaseType::InternalRelease();
		}

		ComPtr & operator=(const ComPtr & right) noexcept
		{
			ComPtr(right).Swap(*this);
			return *this;
		}

		template <typename U>
		ComPtr & operator=(const ComPtr<U> & right) noexcept
		{
			ComPtr(right).Swap(*this);
			return *this;
		}

		void Reset() noexcept
		{
			ComPtr().Swap(*this);
		}

		template<typename U>
		void Reset(U * other)
		{
			ComPtr(other).Swap(*this);
		}

		void Swap(ComPtr & other) noexcept
		{
			BaseType::Swap(other);
		}

		using BaseType::Get;

		// this works with undefined classes seems its evaluated at usage site when itf is defined
		//	-> typename Detail::Restricted<T>* operator->() const noexcept

		// but the following fails when compiling a header, assume its compiler specific as to when its evaluated
		// current trick is to use COM_PTR_FWD to prevent is_final being evaluated
		typename Detail::CanRestrict<T>::Type* operator->() const noexcept
		{
			assert(Get() != nullptr);
			return static_cast<typename Detail::CanRestrict<T>::Type*>(BaseType::Get());
		}

		bool Unique() const noexcept
		{
			return BaseType::UseCount() == 1;
		}

		explicit operator bool() const noexcept
		{
			return BaseType::Get() != nullptr;
		}

		T** operator&() noexcept // NOLINT(google-runtime-operator) use transfer semantics
		{
			assert(Get() == nullptr);
			return const_cast<T**>(BaseType::GetAddress());
		}

		HRESULT CreateInstance(const IID & iid) noexcept
		{
			assert(Get() == nullptr); // or just call reset?
			return ::CoCreateInstance(iid, nullptr, CLSCTX_ALL, __uuidof(T), reinterpret_cast<void**>(operator&()));
		}

		template<typename U>
		HRESULT Detach(U** target)
		{
			if (!target)
			{
				assert(false);
				return E_POINTER;
			}
			*target = BaseType::InternalDetach();
			return S_OK;
		}

		template<typename U>
		static ComPtr Attach(U * value)
		{
			return ComPtr(value, false);
		}

		bool operator==(const ComPtr & other) const
		{
			// consider Com Equality? consider template<typename U> compare
			return other.Get() == Get();
		}

		bool operator!=(const ComPtr &other) const
		{
			return !(*this == other);
		}

	private:
		ComPtr(T * other, bool ignored) noexcept
		{
			(void) ignored;
			BaseType::InternalAttach(other);
		}
	};

	// wrl uses nothrow new and passes args supposedly to prevent leaks (and also to return out of mem HR)
	// however c++ should not leak from well written ctors!? and badalloc should be caught and returned as an HR
	// possible can make ctors private and friends of make to have better control over internal ref count?
	//template <typename T, typename I, typename... Args>
	//inline ComPtr<I> Make(Args&&... args)
	//{
	//	// static_assert(T is a SimpleCom::Unknown)? or override
	//	T* t = new T(std::forward<Args>(args)...); // ref==0
	//	return ComPtr<I>(static_cast<I*>(t)); // ret with +1 ref
	//}

	template <typename T, typename... Args>
	ComPtr<T> MakeConcrete(Args&&... args)
	{
		return ComPtr<T>::Attach(new T(std::forward<Args>(args)...));
	}

	template <typename T, typename I, typename... Args>
	ComPtr<I> Make(Args&&... args)
	{
		return ComPtr<I>::Attach(static_cast<I*>(new T(std::forward<Args>(args)...)));
	}

	template <typename T, typename... Args>
	ComPtr<typename T::DefaultInterface> Make(Args&&... args)
	{
		return Make<T, typename T::DefaultInterface>(std::forward<Args>(args)...);
	}
}