#pragma once

#include <GLib/StackOrHeap.h>
#include <GLib/Win/ErrorCheck.h>

#include <functional>

namespace GLib::Win
{
	namespace Detail
	{
		inline std::wstring GetWindowText(HWND const hWnd)
		{
			GLib::Util::WideCharBuffer s;
			SetLastError(ERROR_SUCCESS); // GetWindowTextLength does not set last error on success
			size_t lengthWithoutTerminator = GetWindowTextLengthW(hWnd);
			Util::AssertTrue(lengthWithoutTerminator != 0 || GetLastError() == 0, "GetWindowTextLengthW");
			if (lengthWithoutTerminator != 0)
			{
				s.EnsureSize(lengthWithoutTerminator + 1);
				lengthWithoutTerminator = ::GetWindowTextW(hWnd, s.Get(), static_cast<int>(s.Size()));
				Util::AssertTrue(lengthWithoutTerminator != 0 || GetLastError() == 0, "GetWindowTextW");
			}
			return s.Get();
		}
	}

	class WindowFinder
	{
		using WindowEnumerator = std::function<BOOL(HWND)>;

		static BOOL CALLBACK EnumWindowsCallback(HWND const handle, LPARAM const param) noexcept
		{
			// todo setLastError when return false
			return (*Util::Detail::WindowsCast<WindowEnumerator const *>(param))(handle);
		}

	public:
		static HWND Find(ULONG const pid, std::string const & windowText)
		{
			HDESK const desktop = GetThreadDesktop(GetCurrentThreadId());
			Util::AssertTrue(desktop != nullptr, "GetThreadDesktop");
			auto const wideWindowText = Cvt::A2W(windowText);

			HWND ret {};
			WindowEnumerator func = [&](HWND const wnd) noexcept -> BOOL
			{
				ULONG windowPid = 0;
				GetWindowThreadProcessId(wnd, &windowPid);

				try
				{
					auto const wt = Detail::GetWindowText(wnd);
					if (windowPid == pid && wt == wideWindowText) // duplicate names?
					{
						ret = wnd;
					}
				}
				catch (std::exception const &)
				{}
				return TRUE;
			};

			BOOL const result = EnumDesktopWindows(desktop, EnumWindowsCallback, Util::Detail::WindowsCast<LPARAM>(&func));
			Util::AssertTrue(result, "EnumDesktopWindows");
			return ret;
		}
	};
}