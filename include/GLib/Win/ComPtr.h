#pragma once

#include <GLib/Win/Transfer.h>
#include <GLib/Win/WinException.h>

#ifdef COM_PTR_DEBUG
#include <GLib/Win/DebugStream.h>
#endif

#include <objbase.h>

#include <memory>
#include <type_traits>

namespace GLib::Win
{
	template <typename T>
	class ComPtr;

	void CheckHr(HRESULT result, std::string_view message);
	void WarnHr(HRESULT result, std::string_view message) noexcept;

	template <typename T>
	auto GetUuId()
	{
		return __uuidof(T); // NOLINT(clang-diagnostic-language-extension-token) required
	}

	namespace ComPtrDetail
	{
		constexpr ULONG ContextAll = CLSCTX_ALL; // NOLINT bad macro

		template <typename T>
		class Restricted : public T
		{
		public:
			Restricted() = delete;
			Restricted(Restricted const &) = delete;
			Restricted(Restricted &&) = delete;
			Restricted & operator=(Restricted const &) = delete;
			Restricted & operator=(Restricted &&) = delete;
			virtual ~Restricted() = delete;

		private:
			ULONG STDMETHODCALLTYPE AddRef() override
			{
				return 0;
			}

			ULONG STDMETHODCALLTYPE Release() override
			{
				return 0;
			}
		};
	}

	template <typename T>
	auto GetAddress(ComPtr<T> & value) noexcept
	{
		return Transfer<ComPtr<T>, T *>(value);
	}

	template <typename Target, typename Source>
	ComPtr<Target> ComCast(Source const & source)
	{
		ComPtr<Target> value;
		if (source)
		{
			CheckHr(source->QueryInterface(GetUuId<Target>(), GetAddress(value).Void()), "QueryInterface");
		}
		return value;
	}

	template <typename T>
	class ComPtr
	{
		template <typename U>
		friend class ComPtr;

		template <typename U>
		friend U * Get(ComPtr<U> & value) noexcept; // NOLINT(readability-redundant-declaration) (bug: needed but clang-tidy -fix deletes this line)

		template <typename U>
		friend U const *
		Get(ComPtr<U> const & value) noexcept; // NOLINT(readability-redundant-declaration) (bug: needed but clang-tidy -fix deletes this)

		T * ptr {};

	public:
		ComPtr() noexcept = default;

		explicit ComPtr(std::nullptr_t) noexcept {}

		template <typename U>
		explicit ComPtr(U * value) noexcept
			: ptr(InternalAddRef(value))
		{}

		template <typename U>
		explicit ComPtr(ComPtr<U> const & value) noexcept
			: ptr(InternalAddRef(value.ptr))
		{}

		ComPtr(ComPtr const & value) noexcept
			: ptr(InternalAddRef(value.ptr))
		{}

		template <typename U>
		explicit ComPtr(ComPtr<U> && value) noexcept
			: ptr(std::exchange(value.ptr, nullptr))
		{}

		ComPtr(ComPtr && value) noexcept
			: ptr(std::exchange(value.ptr, nullptr))
		{}

		~ComPtr() noexcept
		{
			InternalRelease(std::exchange(ptr, nullptr));
		}

		ComPtr & operator=(nullptr_t) noexcept
		{
			ComPtr {}.Swap(*this);
			return *this;
		}

		ComPtr & operator=(ComPtr const & value) noexcept // NOLINT(bugprone-unhandled-self-assignment,cert-oop54-cpp) clang-tidy bug?
		{
			ComPtr {value}.Swap(*this);
			return *this;
		}

		template <typename U>
		ComPtr & operator=(ComPtr<U> const & value) noexcept
		{
			ComPtr {value}.Swap(*this);
			return *this;
		}

		ComPtr & operator=(ComPtr && value) noexcept
		{
			ComPtr {std::move(value)}.Swap(*this);
			return *this;
		}

		template <typename U>
		ComPtr & operator=(ComPtr<U> && value) noexcept
		{
			ComPtr {std::move(value)}.Swap(*this);
			return *this;
		}

		ComPtrDetail::Restricted<T> * operator->() const
		{
			if (*this)
			{
				return static_cast<ComPtrDetail::Restricted<T> *>(ptr);
			}
			throw std::runtime_error("nullptr");
		}

		explicit operator bool() const noexcept
		{
			return ptr != nullptr;
		}

		static ComPtr Attach(T * value)
		{
			return ComPtr {value, {}};
		}

	private:
		ComPtr(T * value, bool const ignored) noexcept
			: ptr(value)
		{
			static_cast<void>(ignored);
		}

		void Swap(ComPtr & rhs)
		{
			std::swap(ptr, rhs.ptr);
		}

		template <typename U>
		static U * InternalAddRef(U * value) noexcept
		{
			return value ? value->AddRef(), value : value;
		}

		template <typename U>
		static void InternalRelease(U * value) noexcept
		{
			if (value && value->Release() == 0)
			{
#ifdef COM_PTR_DEBUG
				Debug::Stream() << typeid(T).name() << " deleted" << std::endl;
#endif
			}
		}
	};

	template <class T>
	bool operator==(ComPtr<T> const & value, nullptr_t) noexcept
	{
		return !value;
	}

	template <class T>
	bool operator==(nullptr_t, ComPtr<T> const & value) noexcept
	{
		return !value;
	}

	template <class T>
	bool operator!=(ComPtr<T> const & value, nullptr_t) noexcept
	{
		return !!value;
	}

	template <class T>
	bool operator!=(nullptr_t, ComPtr<T> const & value) noexcept
	{
		return !!value;
	}

	template <typename T>
	T * Get(ComPtr<T> & value) noexcept
	{
		return value.ptr;
	}

	template <typename T>
	T const * Get(ComPtr<T> const & value) noexcept
	{
		return value.ptr;
	}

	template <typename T, typename I, typename... Args>
	ComPtr<I> Make(Args &&... args)
	{
		return ComPtr<I>::Attach(static_cast<I *>(std::make_unique<T>(args...).release()));
	}

	template <typename T, typename... Args>
	ComPtr<typename T::DefaultInterface> Make(Args &&... args)
	{
		return Make<T, typename T::DefaultInterface>(std::forward<Args>(args)...);
	}

	template <typename T1, typename T2>
	bool operator==(ComPtr<T1> const & value1, ComPtr<T2> const & value2)
	{
		return Get(ComCast<IUnknown>(value1)) == Get(ComCast<IUnknown>(value2));
	}
}
