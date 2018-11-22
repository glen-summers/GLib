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
		}

		typedef std::unique_ptr<void, Detail::HandleCloser> Handle;
	}
}
