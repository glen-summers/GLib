#pragma once

#include <windowsx.h>

#include "GLib/formatter.h"
#include "GLib/Win/ErrorCheck.h"

#include <functional>
#include <memory>

namespace GLib::Win
{
	class Window;

	namespace Detail
	{
		struct WindowDestroyer
		{
			void operator()(HWND hWnd) const noexcept
			{
				Util::WarnAssertTrue(::DestroyWindow(hWnd), "DestroyWindow failed");
			}
		};

		using WindowHandle = std::unique_ptr<HWND__, Detail::WindowDestroyer>;

		class ClassInfoStore
		{
		public:
			static std::string Register(int icon, int menu, WNDPROC proc)
			{
				(void)icon;
				(void)menu;
				// hash+more
				return Formatter::Format("GTL:{0}", proc);
			}
		};

		inline HINSTANCE Instance()
		{
			return reinterpret_cast<HINSTANCE>(&__ImageBase); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) required
		}

		inline std::string RegisterClass(int icon, int menu, WNDPROC proc)
		{
			std::wstring className = Cvt::a2w(ClassInfoStore::Register(icon, menu, proc));
			auto instance = Instance();

			WNDCLASSEXW wcex = {};
			BOOL exists = ::GetClassInfoExW(instance, className.c_str(), &wcex);
			if (exists == 0)
			{
				Util::AssertTrue(::GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "GetClassInfoEx failed");

				WNDCLASSEXW wc =
				{
					sizeof(WNDCLASSEXW),
					CS_HREDRAW | CS_VREDRAW /*CS_DBLCLKS*/,
					static_cast<WNDPROC>(proc),
					0, 0, instance,
					::LoadIconW(instance, MAKEINTRESOURCE(icon)),
					::LoadCursorW(nullptr, IDC_ARROW),
					reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) required
					MAKEINTRESOURCEW(menu),
					className.c_str()
					// hIconSm etc.
				};

				Util::AssertTrue(::RegisterClassExW(&wc) != 0, "RegisterClass");
			}
			return Cvt::w2a(className);
		}

		inline void AssociateHandle(Window * value, HWND handle)
		{
			::SetLastError(ERROR_SUCCESS); // SetWindowLongPtr does not set last error on success
			auto ret = ::SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(value)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) required
			Util::AssertTrue(ret != 0 || ::GetLastError() == ERROR_SUCCESS, "SetWindowLongPtr");
		}

		inline Window * FromHandle(HWND hWnd)
		{
			return reinterpret_cast<Window*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) required
		}

		inline WindowHandle Create(DWORD style, int icon, int menu, const std::string & title, WNDPROC proc, Window * param)
		{
			std::string className = RegisterClass(icon, menu, proc);
			Detail::WindowHandle handle(::CreateWindowExW(0, Cvt::a2w(className).c_str(), Cvt::a2w(title).c_str(), style, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, Instance(), param));
			Util::AssertTrue(!!handle, "CreateWindow");
			AssociateHandle(param, handle.get());
			//MessageDebugWriteContext("Window create", handle.get(), param);
			return move(handle);
		}

		inline std::wstring GetWindowText(HWND hWnd)
		{
			GLib::Util::WideCharBuffer s;
			::SetLastError(ERROR_SUCCESS); // GetWindowTextLength does not set last error on success
			size_t lengthWithoutTerminator = ::GetWindowTextLengthW(hWnd);
			Util::AssertTrue(lengthWithoutTerminator != 0 || ::GetLastError() == 0, "GetWindowTextLength");
			if (lengthWithoutTerminator != 0)
			{
				s.EnsureSize(lengthWithoutTerminator + 1);
				lengthWithoutTerminator = ::GetWindowTextW(hWnd, s.Get(), static_cast<int>(s.size()));
				Util::AssertTrue(lengthWithoutTerminator != 0 || ::GetLastError() == 0, "GetWindowText");
			}
			return s.Get();
		}
	}

	class WindowFinder // move
	{
		using WindowEnumerator = std::function<bool(HWND)>;

		static BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM param) noexcept
		{
			return (*reinterpret_cast<const WindowEnumerator*>(param))(handle) ? TRUE : FALSE;
			// set error when false
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

			BOOL result = ::EnumDesktopWindows(desktop, EnumWindowsCallback, reinterpret_cast<LPARAM>(&func));
			Util::AssertTrue(result, "EnumDesktopWindows");
			return ret;
		}
	};

	// move
	template <typename T> class PointT
	{
		T x{};
		T y{};

	public:
		PointT() = default;

		PointT(T x, T y) : x(x), y(y)
		{}

		T X() const { return x; }
		T Y() const { return y; }

		bool operator==(const PointT & other) const
		{
			return x == other.x && y == other.y;
		}

		bool operator!=(const PointT & other) const
		{
			return !(*this == other);
		}
	};

	using Point = PointT<int>;

	// move
	class Size
	{
		int x;
		int y;

	public:
		Size()
			: x(0), y(0)
		{}

		Size(int xSize, int ySize)
			: x(xSize), y(ySize)
		{}

		int X() const { return x; }
		int Y() const { return y; }

		bool operator==(const Size & other) const
		{
			return x == other.x && y == other.y;
		}

		bool operator!=(const Size & other) const
		{
			return !(*this == other);
		}

		explicit operator const SIZE &() const { return reinterpret_cast<const SIZE &>(*this); }
		explicit operator const POINT &() const { return reinterpret_cast<const POINT &>(*this); }

		SIZE* operator &() { return reinterpret_cast<SIZE*>(this); }

		Size operator/(int rhs) const
		{
			return Size(x / rhs, y / rhs);
		}
	};

	enum class CloseResult { Allow, Prevent };

	class Window
	{
		Detail::WindowHandle handle;
		HACCEL hAccel{};

	public:
		Window(int icon, int menu, const std::string & title)
			: handle{Detail::Create(WS_OVERLAPPEDWINDOW, icon, menu, title, WindowProc, this)}
		{}

		int PumpMessages() const
		{
			MSG msg = {};
			while (::GetMessageW(&msg, nullptr, 0, 0))
			{
				if (!::TranslateAcceleratorW(handle.get(), hAccel, &msg))
				{
					::TranslateMessage(&msg);
					::DispatchMessageW(&msg);
				}
			}
			return static_cast<int>(msg.wParam);
		}

		bool Show(int cmd) const
		{
			return ::ShowWindow(handle.get(), cmd) != FALSE;
		}

		void Destroy() const
		{
			::DestroyWindow(handle.get());
		}

		int MessageBox(const std::string & message, const std::string & caption, UINT type = MB_OK) const
		{
			int result = ::MessageBoxW(handle.get(), Cvt::a2w(message).c_str(), Cvt::a2w(caption).c_str(), type);
			Util::AssertTrue(result != 0, "MessageBox failed");
			return result;
		}

		template<typename R, typename P>
		void SetTimer(const std::chrono::duration<R, P> & interval, UINT_PTR id = 1) const
		{
			auto count = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
			DWORD milliseconds = GLib::Util::checked_cast<DWORD>(count);
			UINT_PTR timerResult = ::SetTimer(handle.get(), id, milliseconds, nullptr);
			Util::AssertTrue(timerResult != 0, "SetTimer");
		}

		void KillTimer(UINT_PTR id = 1) const
		{
			Util::AssertTrue(::KillTimer(handle.get(), id), "KillTimer");
		}

	protected:
		virtual bool OnSize(const Size &) noexcept { return false; }
		virtual bool OnGetMinMaxInfo(MINMAXINFO &) noexcept { return false; }
		virtual bool OnCommand(int /*command*/) noexcept { return false; }
		virtual CloseResult OnClose() noexcept { return CloseResult::Allow; }
		virtual void OnDestroy() noexcept {}
		virtual void OnNCDestroy() noexcept {}
		virtual bool OnUser(WPARAM, LPARAM) noexcept { return false; }
		virtual bool OnNotify(const NMHDR&) noexcept { return false; }
		virtual bool OnChar(int) noexcept { return false; }
		virtual bool OnKeyDown(int) noexcept { return false; }
		virtual bool OnLeftButtonDown(const Point & ) noexcept { return false; }
		virtual bool OnLeftButtonUp(const Point & ) noexcept { return false; }
		virtual bool OnContextMenu(const Point & ) noexcept { return false; }
		virtual bool OnMouseMove(const Point &) noexcept { return false; }
		virtual bool OnMouseWheel(const Point &, int /*delta*/) noexcept { return false; }
		virtual bool OnCaptureChanged() noexcept { return false; }
		virtual bool OnGetDlgCode(int /*key*/, const MSG&, LRESULT &) { return false; }
		virtual bool OnPaint() noexcept { return false; }
		virtual bool OnTimer() noexcept { return false; }
		virtual bool OnEraseBackground() noexcept { return false; }

		bool OnMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT & result) noexcept
		{
			switch (message)
			{
				case WM_PAINT:
				{
					return OnPaint();
				}

				case WM_ERASEBKGND:
				{
					bool handled = OnEraseBackground();
					if (handled)
					{
						result = 1;
					}
					return handled;
				}

				case WM_SIZE:
				{
					return OnSize({ LOWORD(lParam), HIWORD(lParam) });
				}

				case WM_GETMINMAXINFO:
				{
					return OnGetMinMaxInfo(*reinterpret_cast<MINMAXINFO *>(lParam));
				}

				case WM_COMMAND:
				{
					return OnCommand(LOWORD(wParam));
				}

				case WM_CLOSE:
				{
					return OnClose() == CloseResult::Prevent;
				}

				case WM_DESTROY:
				{
					OnDestroy();
					return false;
				}

				case WM_NCDESTROY:
				{
					OnNCDestroy();
					handle.release(); // ??
					return false;
				}

				case WM_USER:
				{
					return OnUser(wParam, lParam);
				}

				case WM_NOTIFY:
				{
					return OnNotify(*reinterpret_cast<const NMHDR*>(lParam));
				}

				case WM_CHAR:
				{
					return OnChar(static_cast<int>(wParam));
				}

				case WM_KEYDOWN:
				{
					return OnKeyDown(static_cast<int>(wParam));
				}

				case WM_LBUTTONDOWN:
				{
					return OnLeftButtonDown({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
				}

				case WM_LBUTTONUP:
				{
					return OnLeftButtonUp({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
				}

				case WM_CONTEXTMENU:
				{
					return OnContextMenu({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
				}

				case WM_MOUSEMOVE:
				{
					return OnMouseMove({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
				}

				case WM_MOUSEWHEEL:
				{
					//WORD keys = GET_KEYSTATE_WPARAM(wParam);
					return OnMouseWheel({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) }, GET_WHEEL_DELTA_WPARAM(wParam));
				}

				case WM_CAPTURECHANGED:
				{
					return OnCaptureChanged();
				}

				case WM_GETDLGCODE:
				{
					return OnGetDlgCode(static_cast<int>(wParam), *reinterpret_cast<MSG*>(lParam), result);
				}

				case WM_TIMER:
				{
					return OnTimer();
				}

				default:;
			}

			return false;
		}

	private:
		static LRESULT APIENTRY WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
		{
			LRESULT result = 0;
			Window * window = Detail::FromHandle(hWnd);
			bool handled = window && window->OnMessage(message, wParam, lParam, result);
			result = handled ? result : ::DefWindowProc(hWnd, message, wParam, lParam);
			//MessageDebugWrite(FromHandle(hWnd), hWnd, message, wParam, lParam, result);
			return result;
		}
	};
}
