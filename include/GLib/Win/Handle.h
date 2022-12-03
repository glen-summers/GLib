#pragma once

#include <GLib/Win/ErrorCheck.h>

#include <cassert>
#include <memory>

namespace GLib::Win
{
	using AccelBase = std::remove_pointer_t<HACCEL>;
	using HandleBase = std::remove_pointer_t<HANDLE>;
	using IconBase = std::remove_pointer_t<HICON>;
	using InstanceBase = std::remove_pointer_t<HINSTANCE>;
	using WindowHandleBase = std::remove_pointer_t<HWND>;
	using WndProcBase = std::remove_pointer_t<WNDPROC>;
	using KeyBase = std::remove_pointer_t<HKEY>;
	using DeskBase = std::remove_pointer_t<HDESK>;

	struct HandleCloser
	{
		void operator()(HandleBase * const h) const noexcept
		{
			// h!= INVALID_HANDLE_VALUE, or null - via policy?
			// should not be needed, null should never get here as its a unique_ptr
			// and INVALID_HANDLE_VALUE appears to return success from CloseHandle
			assert(h != nullptr && h != INVALID_HANDLE_VALUE); // NOLINT bad macro
			Util::WarnAssertTrue(CloseHandle(h), "CloseHandle");
		}
	};

	using Handle = std::unique_ptr<HandleBase, HandleCloser>;
	using WindowHandle = std::unique_ptr<WindowHandleBase>;
}