#pragma once

#include "GLib/Win/ErrorCheck.h"

#include <memory>
#include <cassert>

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			struct HandleCloser
			{
				void operator()(void* h) const noexcept
				{
					// h!= INVALID_HANDLE_VALUE, or null - via policy?
					// should not be needed, null should never get here as its a unique_ptr
					// and INVALID_HANDLE_VALUE appears to return success from CloseHandle
					assert(h != nullptr && h != INVALID_HANDLE_VALUE);
					Util::WarnAssertTrue(::CloseHandle(h), "CloseHandle");
				}
			};

			struct LocalFreer
			{
				void operator()(HLOCAL p) const noexcept
				{
					assert(p != nullptr);
					Util::WarnAssertTrue(::LocalFree(p) == nullptr, "LocalFree");
				}
			};

			template <typename T, typename Ptr>  struct Transfer
			{
				Ptr & local;
				T * value;

				Transfer(Ptr & local) : local(local), value()
				{}

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

		typedef std::unique_ptr<void, Detail::HandleCloser> Handle;

		typedef std::unique_ptr<void, Detail::LocalFreer> LocalPtr;

		template <typename T>  class Local
		{
			LocalPtr p;

		public:
			Local(T * value = nullptr) : p(value)
			{}

			const T * Get() const
			{
				return reinterpret_cast<T*>(p.get());
			}

			Detail::Transfer<T, LocalPtr> GetPtr()
			{
				return {p};
			}
		};
	}
}
