#pragma once

#include "GLib/Win/ErrorCheck.h"

#include <cassert>

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			struct LocalFreer
			{
				void operator()(HLOCAL p) const noexcept
				{
					assert(p != nullptr);
					Util::WarnAssertTrue(::LocalFree(p) == nullptr, "LocalFree");
				}
			};

			template <typename T, typename LocalPtr> struct Transfer
			{
				LocalPtr & local;
				T * value;

				Transfer(LocalPtr & local) : local(local), value()
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
			using Ptr = std::unique_ptr<T, Detail::LocalFreer>;
			Ptr p;

		public:
			Local(T * value = nullptr) : p(value)
			{}

			const T * Get() const
			{
				return p.get();
			}

			Detail::Transfer<T, Ptr> GetPtr()
			{
				return {p};
			}
		};
	}
}
