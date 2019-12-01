#pragma once

#include <GLib/Win/ErrorCheck.h>

#include <cassert>
#include <memory>

namespace GLib::Win
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
				assert(h != nullptr && h != INVALID_HANDLE_VALUE);  // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
				Util::WarnAssertTrue(::CloseHandle(h), "CloseHandle");
			}
		};
	}

	using Handle = std::unique_ptr<void, Detail::HandleCloser>;
}