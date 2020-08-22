#pragma once

#include <windowsx.h>

#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/Painter.h>

#ifdef GLIB_DEBUG_MESSAGES
#include <GLib/Win/MessageDebug.h>
#endif

#include <GLib/formatter.h>

#include <functional>
#include <memory>

namespace GLib::Win
{
	class Window;

	struct Size : SIZE {};
	struct Point : POINT {};
	struct Rect : RECT {};

	namespace Detail
	{
		template <typename T1, typename T2> T1 Munge(T2 t2)
		{
			return reinterpret_cast<T1>(t2); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) many windows casts from LPARAMs etc.
		}

		constexpr unsigned int HRedraw = CS_HREDRAW;
		constexpr unsigned int VRedraw = CS_VREDRAW;
		constexpr unsigned int OverlappedWindow = WS_OVERLAPPEDWINDOW; // NOLINT(hicpp-signed-bitwise)

		inline auto MakeIntResource(int id)
		{
			return MAKEINTRESOURCEW(id); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast) bad macro
		}

		inline WORD LoWord(WPARAM param)
		{
			return LOWORD(param);  // NOLINT(hicpp-signed-bitwise)
		}

		inline WORD HiWord(WPARAM param)
		{
			return HIWORD(param);  // NOLINT(hicpp-signed-bitwise)
		}

		inline Point PointFromParam(LPARAM param)
		{
			return { GET_X_LPARAM(param), GET_Y_LPARAM(param) }; // NOLINT(hicpp-signed-bitwise)
		}

		inline short WheelData(WPARAM param)
		{
			return GET_WHEEL_DELTA_WPARAM(param); // NOLINT(hicpp-signed-bitwise)
		}

		inline Size SizeFromParam(LPARAM param)
		{
			return { LoWord(param), HiWord(param) };
		}

		inline HINSTANCE Instance()
		{
			return Detail::Munge<HINSTANCE>(&__ImageBase);
		}

		struct WindowDestroyer
		{
			void operator()(HWND hWnd) const noexcept
			{
				Util::WarnAssertTrue(::DestroyWindow(hWnd), "DestroyWindow");
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
				return Formatter::Format("GTL:{0}", static_cast<void*>(&proc));
			}
		};

		inline std::string RegisterClass(int icon, int menu, WNDPROC proc)
		{
			std::wstring className = Cvt::a2w(ClassInfoStore::Register(icon, menu, proc));
			auto instance = Instance();

			WNDCLASSEXW wcex = {};
			BOOL exists = ::GetClassInfoExW(instance, className.c_str(), &wcex);
			if (exists == 0)
			{
				Util::AssertTrue(::GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "GetClassInfoExW");

				HICON i = icon == 0 ? nullptr : ::LoadIconW(instance, MakeIntResource(icon));

				WNDCLASSEXW wc =
				{
					sizeof(WNDCLASSEXW),
					HRedraw | VRedraw,
					static_cast<WNDPROC>(proc),
					0, 0, instance, i,
					::LoadCursorW(nullptr, IDC_ARROW), // NOLINT(cppcoreguidelines-pro-type-cstyle-cast) baad macro
					Detail::Munge<HBRUSH>(size_t{COLOR_WINDOW} + 1),
					MakeIntResource(menu),
					className.c_str()
					// hIconSm etc.
				};

				Util::AssertTrue(::RegisterClassExW(&wc) != 0, "RegisterClassExW");
			}
			return Cvt::w2a(className);
		}

		inline void AssociateHandle(Window * value, HWND handle)
		{
			::SetLastError(ERROR_SUCCESS); // SetWindowLongPtr does not set last error on success
			auto ret = ::SetWindowLongPtr(handle, GWLP_USERDATA, Detail::Munge<LONG_PTR>(value));
			Util::AssertTrue(ret != 0 || ::GetLastError() == ERROR_SUCCESS, "SetWindowLongPtr");
		}

		inline Window * FromHandle(HWND hWnd)
		{
			return Detail::Munge<Window*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
		}

		inline WindowHandle Create(DWORD style, int icon, int menu, const std::string & title, WNDPROC proc, Window * param)
		{
			std::string className = RegisterClass(icon, menu, proc);
			Detail::WindowHandle handle(::CreateWindowExW(0, Cvt::a2w(className).c_str(), Cvt::a2w(title).c_str(), style, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, Instance(), param));
			Util::AssertTrue(!!handle, "CreateWindowExW");
			AssociateHandle(param, handle.get());

#ifdef GLIB_DEBUG_MESSAGES
			GLib::Win::MessageDebug::Write("Window create", handle.get(), param);
#endif

			return handle;
		}

		inline HACCEL LoadAccel(int id)
		{
			HACCEL accel = ::LoadAcceleratorsW(Instance(), MakeIntResource(id));
			Util::AssertTrue(accel != nullptr, "LoadAcceleratorsW");
			return accel;
		}
	}

	enum class CloseResult { Allow, Prevent };

	class Window
	{
		Detail::WindowHandle handle;
		HACCEL accel{};

	public:
		Window(int icon, int menu, int accel, const std::string & title)
			: handle{Detail::Create(Detail::OverlappedWindow, icon, menu, title, WindowProc, this)}
			, accel{accel != 0 ? Detail::LoadAccel(accel) : nullptr}
		{}

		int PumpMessages() const
		{
			MSG msg = {};
			while (::GetMessageW(&msg, nullptr, 0, 0) != FALSE) // returns -1 on error?
			{
				auto ret = ::TranslateAcceleratorW(handle.get(), accel, &msg); // no error if hAccel is null
				if (ret == 0)
				{
					::TranslateMessage(&msg);
					::DispatchMessageW(&msg);
				}
			}
			return static_cast<int>(msg.wParam);
		}

		Size ClientSize() const
		{
			RECT rc;
			Util::AssertTrue(::GetClientRect(handle.get(), &rc), "GetClientRect");
			return {rc.right - rc.left, rc.bottom - rc.top};
		}

		bool Show(int cmd) const
		{
			return ::ShowWindow(handle.get(), cmd) != FALSE;
		}

		Painter GetPainter() const
		{
			PAINTSTRUCT ps{};
			auto dc = ::BeginPaint(handle.get(), &ps);
			return Painter{PaintInfo{ps, handle.get(), dc }};
		}

		void Close() const
		{
			Send(WM_CLOSE);
		}

		void Destroy() const
		{
			::DestroyWindow(handle.get());
		}

		int MessageBox(const std::string & message, const std::string & caption, UINT type = MB_OK) const
		{
			int result = ::MessageBoxW(handle.get(), Cvt::a2w(message).c_str(), Cvt::a2w(caption).c_str(), type);
			Util::AssertTrue(result != 0, "MessageBoxW");
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

		void Invalidate(bool erase) const
		{
			Util::AssertTrue(::InvalidateRect(Handle(), nullptr, erase ? TRUE : FALSE), "InvalidateRect");
		}

		static void SetHandled(bool newValue = true)
		{
			SetHandled(newValue, true);
		}

		static bool GetHandled()
		{
			return SetHandled(false, false);
		}

	private:
		static bool SetHandled(bool newValue, bool exchange)
		{
			static thread_local bool handled;
			return exchange ? std::exchange(handled, newValue) : handled;
		}

		virtual void OnSize(const Size & /**/) noexcept {}
		virtual void OnGetMinMaxInfo(MINMAXINFO & /**/) noexcept {}
		virtual void OnCommand(int /*command*/) noexcept {}
		virtual CloseResult OnClose() noexcept { return CloseResult::Allow; }
		virtual void OnDestroy() noexcept {}
		virtual void OnNCDestroy() noexcept {}
		virtual void OnUser(WPARAM /*w*/, LPARAM /*p*/) noexcept {}
		virtual void OnNotify(const NMHDR & /*hdr*/) noexcept {}
		virtual void OnChar(int /*char*/) noexcept {}
		virtual void OnKeyDown(int /*key*/) noexcept {}
		virtual void OnLeftButtonDown(const Point & /**/) noexcept {}
		virtual void OnLeftButtonUp(const Point & /**/) noexcept {}
		virtual void OnContextMenu(const Point & /**/) noexcept {}
		virtual void OnMouseMove(const Point & /**/) noexcept {}
		virtual void OnMouseWheel(const Point & /**/, int /*delta*/) noexcept {}
		virtual void OnCaptureChanged() noexcept {}
		virtual void OnGetDlgCode(int /*key*/, const MSG & /**/, LRESULT & /**/) {}
		virtual void OnPaint() noexcept {}
		virtual void OnTimer() noexcept {}
		virtual void OnEraseBackground() noexcept {}

		void OnMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT & result) noexcept
		{
			switch (message)
			{
				case WM_PAINT:
				{
					return OnPaint();
				}

				case WM_ERASEBKGND:
				{
					OnEraseBackground();
					if (!GetHandled())
					{
						result = 1;
					}
					return;
				}

				case WM_SIZE:
				{
					return OnSize(Detail::SizeFromParam(lParam));
				}

				case WM_GETMINMAXINFO:
				{
					return OnGetMinMaxInfo(*Detail::Munge<MINMAXINFO *>(lParam));
				}

				case WM_COMMAND:
				{
					return OnCommand(Detail::LoWord(wParam));
				}

				case WM_CLOSE:
				{
					if (OnClose() == CloseResult::Prevent)
					{
						SetHandled();
					}
					return;
				}

				case WM_DESTROY:
				{
					return OnDestroy();
				}

				case WM_NCDESTROY:
				{
					OnNCDestroy();
					(void)handle.release();
					return;
				}

				case WM_USER:
				{
					return OnUser(wParam, lParam);
				}

				case WM_NOTIFY:
				{
					return OnNotify(*Detail::Munge<const NMHDR*>(lParam));
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
					return OnLeftButtonDown(Detail::PointFromParam(lParam));
				}

				case WM_LBUTTONUP:
				{
					return OnLeftButtonUp(Detail::PointFromParam(lParam));
				}

				case WM_CONTEXTMENU:
				{
					return OnContextMenu(Detail::PointFromParam(lParam));
				}

				case WM_MOUSEMOVE:
				{
					return OnMouseMove(Detail::PointFromParam(lParam));
				}

				case WM_MOUSEWHEEL:
				{
					//WORD keys = GET_KEYSTATE_WPARAM(wParam);
					return OnMouseWheel(Detail::PointFromParam(lParam), Detail::WheelData(wParam));
				}

				case WM_CAPTURECHANGED:
				{
					return OnCaptureChanged();
				}

				case WM_GETDLGCODE:
				{
					return OnGetDlgCode(static_cast<int>(wParam), *Detail::Munge<MSG*>(lParam), result);
				}

				case WM_TIMER:
				{
					return OnTimer();
				}

				default:
				{
					SetHandled(false);
				}
			}
		}

		protected:
			HWND Handle() const
			{
				return handle.get();
			}

			static HINSTANCE Instance()
			{
				return Detail::Instance();
			}

			LRESULT Send(UINT msg, WPARAM wParam = {}, LPARAM lParam = {}) const
			{
				return ::SendMessageW(Handle(), msg, wParam, lParam);
			}

	private:
		static LRESULT APIENTRY WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
		{
			LRESULT result = 0;
			Window * window = Detail::FromHandle(hWnd);
			bool handled{};
			if (window != nullptr)
			{
				auto oldValue = SetHandled(false, true);
				window->OnMessage(message, wParam, lParam, result);
				handled = SetHandled(oldValue, true);
			}

			if (!handled)
			{
				result = ::DefWindowProc(hWnd, message, wParam, lParam);
			}

#ifdef GLIB_DEBUG_MESSAGES
			MessageDebug::Write(Detail::FromHandle(hWnd), hWnd, message, wParam, lParam, result);
#endif

			return result;
		}
	};
}
