#pragma once

#include "GLib/Win/ErrorCheck.h"
#include "GLib/stackorheap.h"

#include <functional>

namespace GLib::Win
{
	namespace Detail
	{
		inline std::wstring GetWindowText(HWND hWnd)
		{
			GLib::Util::WideCharBuffer s;
			::SetLastError(ERROR_SUCCESS); // GetWindowTextLength does not set last error on success
			size_t lengthWithoutTerminator = ::GetWindowTextLengthW(hWnd);
			Util::AssertTrue(lengthWithoutTerminator != 0 || ::GetLastError() == 0, "GetWindowTextLengthW");
			if (lengthWithoutTerminator != 0)
			{
				s.EnsureSize(lengthWithoutTerminator + 1);
				lengthWithoutTerminator = ::GetWindowTextW(hWnd, s.Get(), static_cast<int>(s.size()));
				Util::AssertTrue(lengthWithoutTerminator != 0 || ::GetLastError() == 0, "GetWindowTextW");
			}
			return s.Get();
		}
	}

	class WindowFinder
	{
		using WindowEnumerator = std::function<bool(HWND)>;

		static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM param) noexcept
		{
			// todo setlasterror when return false
			return (*reinterpret_cast<const WindowEnumerator*>(param))(handle) ? TRUE : FALSE; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) required
		}

	public:
		static HWND Find(DWORD pid, const std::string & windowText)
		{
			HDESK desktop = ::GetThreadDesktop(::GetCurrentThreadId());
			Util::AssertTrue(desktop != nullptr, "GetThreadDesktop");
			auto wideWindowText = Cvt::a2w(windowText);

			HWND ret{};
			WindowEnumerator func = [&](HWND wnd) noexcept -> bool
			{
				DWORD windowPid;
				::GetWindowThreadProcessId(wnd, &windowPid);

				try
				{
					auto wt = Detail::GetWindowText(wnd);
					if (windowPid == pid && wt == wideWindowText) // duplicate names?
					{
						ret = wnd;
					}
				}
				catch(const std::exception &)
				{}
				return true;
			};

			BOOL result = ::EnumDesktopWindows(desktop, EnumWindowsCallback, reinterpret_cast<LPARAM>(&func)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
			Util::AssertTrue(result, "EnumDesktopWindows");
			return ret;
		}
	};
}