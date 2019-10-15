#pragma once

#include "GLib/Win/ComErrorCheck.h"
#include "GLib/Win/WinException.h"

#ifdef COM_PTR_DEBUG
#include "GLib/Win/DebugStream.h"
#endif

#include <objbase.h>
#include <type_traits>

#include "cassert"

namespace GLib::Win
{
	template <typename T> class ComPtr;

	template <typename Target, typename Source>
	ComPtr<Target> ComCast(const Source & source)
	{
		ComPtr<Target> value;
		if (source)
		{
			CheckHr(source->QueryInterface(&value), "QueryInterface");
		}
		return value;
	}

	template <typename T>
	class ComPtr
	{
		template <typename> friend class ComPtr;

		T * p {};

	public:
		ComPtr() noexcept = default;

		explicit ComPtr(std::nullptr_t) noexcept
		{}

		template <typename U> explicit ComPtr(U * u) noexcept
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

		~ComPtr() noexcept
		{
			InternalRelease(std::exchange(p, nullptr));
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
		ComPtr & operator=(ComPtr<U>&& right) noexcept
		{
			ComPtr tmp(std::move(right));
			std::swap(tmp.p, p);
			return *this;
		}

		T * Get() const noexcept
		{
			return p;
		}

		// remove? transfer
		T * const * GetAddress() const noexcept
		{
			return &p;
		}

		// restrict?
		T * operator->() const noexcept
		{
			assert(p != nullptr);
			return p;
		}

		explicit operator bool() const noexcept
		{
			return p != nullptr;
		}

		// remove? transfer
		T** operator&() noexcept // NOLINT(google-runtime-operator) use transfer semantics
		{
			assert(p == nullptr);
			return const_cast<T**>(GetAddress());
		}

		template<typename U>
		static ComPtr Attach(U * value)
		{
			return ComPtr(value, false);
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

	template <typename T1, typename T2>
	inline bool operator==(const ComPtr<T1> & t1, const ComPtr<T2>& t2)
	{
		return ComCast<IUnknown>(t1) == ComCast<IUnknown>(t2);
	}
}