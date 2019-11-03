#pragma once

#include "GLib/Win/ErrorCheck.h"

#include <cassert>

namespace GLib::Win
{
	namespace LocalDetail
	{
		struct LocalFreer
		{
			void operator()(HLOCAL p) const noexcept
			{
				Util::WarnAssertTrue(::LocalFree(p) == nullptr, "LocalFree");
			}
		};

		template <typename T, typename LocalPtr> class Transfer
		{
			LocalPtr & local;
			T * value;

		public:
			explicit Transfer(LocalPtr & local) : local(local), value()
			{}

			Transfer() = delete;
			Transfer(const Transfer &) = delete;
			Transfer(Transfer &&) = delete;
			Transfer & operator=(const Transfer &) = delete;
			Transfer & operator=(Transfer &&) = delete;

			~Transfer()
			{
				local.reset(value);
			}

			T** Address()
			{
				return &value;
			}
		};
	}

	template <typename T> class Local
	{
		using Ptr = std::unique_ptr<T, LocalDetail::LocalFreer>;
		Ptr p;

	public:
		explicit Local(T * value = nullptr) : p(value)
		{}

		const T * Get() const
		{
			return p.get();
		}

		LocalDetail::Transfer<T, Ptr> GetPtr()
		{
			return LocalDetail::Transfer<T, Ptr>{static_cast<Ptr&>(p)};
		}
	};
}