#pragma once

#include "GLib/Win/ComErrorCheck.h"
#include "GLib/Win/Transfer.h"
#include "GLib/Win/WinException.h"

#ifdef COM_PTR_DEBUG
#include "GLib/Win/DebugStream.h"
#endif

#include <objbase.h>
#include <type_traits>

namespace GLib::Win
{
	template <typename T> class ComPtr;

	namespace ComPtrDetail
	{
		constexpr DWORD ContextAll = CLSCTX_ALL; // NOLINT(hicpp-signed-bitwise) baad macro

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
			ULONG STDMETHODCALLTYPE AddRef() final;
			ULONG STDMETHODCALLTYPE Release() final;
		};
	}

	template <typename T>
	auto GetAddress(ComPtr<T> & value) noexcept
	{
		return Transfer<ComPtr<T>, T*>(value);
	}

	template <typename Target, typename Source>
	ComPtr<Target> ComCast(const Source & source)
	{
		ComPtr<Target> value;
		if (source)
		{
			CheckHr(source->QueryInterface(__uuidof(Target), GetAddress(value)), "QueryInterface");
		}
		return value;
	}

	template <typename T>
	class ComPtr
	{
		template<typename U> friend class ComPtr;
		template<typename U> friend U * Get(ComPtr<U> & p) noexcept;
		template<typename U> friend const U * Get(const ComPtr<U> & p) noexcept;

		T * p {};

	public:
		ComPtr() noexcept = default;

		explicit ComPtr(std::nullptr_t) noexcept
		{}

		template <typename U>
		explicit ComPtr(U * u) noexcept
			: p(InternalAddRef(u))
		{}

		template <typename U>
		ComPtr(const ComPtr<U> & other) noexcept
			: p(InternalAddRef(other.p))
		{}

		ComPtr(const ComPtr & other) noexcept
			: p(InternalAddRef(other.p))
		{}

		template <typename U>
		ComPtr(ComPtr<U> && right) noexcept
			: p(std::exchange(right.p, nullptr))
		{}

		ComPtr(ComPtr && right) noexcept
			: p(std::exchange(right.p, nullptr))
		{}

		~ComPtr() noexcept
		{
			InternalRelease(std::exchange(p, nullptr));
		}

		ComPtr & operator=(nullptr_t) noexcept
		{
			ComPtr tmp;
			std::swap(tmp.p, p);
			return *this;
		}

		ComPtr & operator=(const ComPtr & right) noexcept
		{
			ComPtr tmp(right);
			std::swap(tmp.p, p);
			return *this;
		}

		template <typename U>
		ComPtr & operator=(const ComPtr<U> & right) noexcept
		{
			ComPtr tmp(right);
			std::swap(tmp.p, p);
			return *this;
		}

		ComPtr & operator=(ComPtr && right) noexcept
		{
			ComPtr tmp(std::move(right));
			std::swap(tmp.p, p);
			return *this;
		}

		template<typename U>
		ComPtr & operator=(ComPtr<U> && right) noexcept
		{
			ComPtr tmp(std::move(right));
			std::swap(tmp.p, p);
			return *this;
		}

		ComPtrDetail::Restricted<T> * operator->() const
		{
			if (*this)
			{
				return static_cast<typename ComPtrDetail::Restricted<T>*>(p);
			}
			throw std::runtime_error("nullptr");
		}

		explicit operator bool() const noexcept
		{
			return p != nullptr;
		}

		static ComPtr Attach(T * value)
		{
			return ComPtr{value, {}};
		}

	private:
		ComPtr(T * other, bool ignored) noexcept
			: p(other)
		{
			(void)ignored;
		}

		template <typename T> static T * InternalAddRef(T * value) noexcept
		{
			return value ? value->AddRef(), value : value;
		}

		template <typename T> static void InternalRelease(T * value) noexcept
		{
			if (value && value->Release() == 0)
			{
#ifdef COM_PTR_DEBUG
				Debug::Stream() << typeid(T).name() << " deleted" << std::endl;
#endif
			}
		}
	};

	template <class T> bool operator==(const ComPtr<T> & p, nullptr_t) noexcept { return !p; }
	template <class T> bool operator==(nullptr_t, const ComPtr<T> & p) noexcept { return !p; }
	template <class T> bool operator!=(const ComPtr<T> & p, nullptr_t) noexcept { return !!p; }
	template <class T> bool operator!=(nullptr_t, const ComPtr<T> & p) noexcept { return !!p; }

	template <typename T> T * Get(ComPtr<T> & p) noexcept
	{
		return p.p;
	}

	template <typename T> const T * Get(const ComPtr<T> & p) noexcept
	{
		return p.p;
	}

	template <typename T, typename I, typename... Args>
	ComPtr<I> Make(Args && ... args)
	{
		return ComPtr<I>::Attach(static_cast<I*>(new T(std::forward<Args>(args)...)));
	}

	template <typename T, typename... Args>
	ComPtr<typename T::DefaultInterface> Make(Args && ... args)
	{
		return Make<T, typename T::DefaultInterface>(std::forward<Args>(args)...);
	}

	template <typename T1, typename T2>
	inline bool operator==(const ComPtr<T1> & t1, const ComPtr<T2> & t2)
	{
		return Get(ComCast<IUnknown>(t1)) == Get(ComCast<IUnknown>(t2));
	}
}