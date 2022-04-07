#pragma once

#include <GLib/StackOrHeap.h>
#include <GLib/Win/ErrorCheck.h>

#include <functional>

namespace GLib::Win
{
	namespace Detail
	{
		inline std::wstring GetWindowText(HWND hWnd)
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
		using WindowEnumerator = std::function<bool(HWND)>;

		static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM param) noexcept
		{
			// todo setLastError when return false
			return (*Util::Detail::WindowsCast<const WindowEnumerator *>(param))(handle);
		}

	public:
		static HWND Find(ULONG pid, const std::string & windowText)
		{
			HDESK desktop = GetThreadDesktop(GetCurrentThreadId());
			Util::AssertTrue(desktop != nullptr, "GetThreadDesktop");
			auto wideWindowText = Cvt::A2W(windowText);

			HWND ret {};
			WindowEnumerator func = [&](HWND wnd) noexcept -> bool
			{
				ULONG windowPid = 0;
				GetWindowThreadProcessId(wnd, &windowPid);

				try
				{
					auto wt = Detail::GetWindowText(wnd);
					if (windowPid == pid && wt == wideWindowText) // duplicate names?
					{
						ret = wnd;
					}
				}
				catch (const std::exception &)
				{}
				return true;
			};

			BOOL result = EnumDesktopWindows(desktop, EnumWindowsCallback, reinterpret_cast<LPARAM>(&func));
			Util::AssertTrue(result, "EnumDesktopWindows");
			return ret;
		}
	};
}