#pragma once

#include <windowsx.h>

#include <GLib/CheckedCast.h>
#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/Painter.h>

#ifdef GLIB_DEBUG_MESSAGES
#include <GLib/Win/MessageDebug.h>
#endif

#include <GLib/Formatter.h>

#include <memory>

namespace GLib::Win
{
	class Window;

	struct Size : SIZE
	{};

	struct Point : POINT
	{};

	struct Rect : RECT
	{};

	namespace Detail
	{
		constexpr unsigned int HRedraw = CS_HREDRAW;
		constexpr unsigned int VRedraw = CS_VREDRAW;
		constexpr unsigned int OverlappedWindow = WS_OVERLAPPEDWINDOW; // NOLINT bad macro

		inline auto MakeIntResource(int const id)
		{
			return MAKEINTRESOURCEW(id); // NOLINT bad macro
		}

		inline WORD LoWord(WPARAM const param)
		{
			return LOWORD(param); // NOLINT bad macro
		}

		inline WORD HiWord(WPARAM const param)
		{
			return HIWORD(param); // NOLINT bad macro
		}

		inline Point PointFromParam(LPARAM const param)
		{
			return {GET_X_LPARAM(param), GET_Y_LPARAM(param)}; // NOLINT bad macro
		}

		inline short WheelData(WPARAM const param)
		{
			return GET_WHEEL_DELTA_WPARAM(param); // NOLINT bad macro
		}

		inline Size SizeFromParam(LPARAM const param)
		{
			return {LoWord(param), HiWord(param)};
		}

		inline HINSTANCE Instance()
		{
			return Util::Detail::WindowsCast<HINSTANCE>(&__ImageBase);
		}

		struct WindowDestroyer
		{
			void operator()(HWND const hWnd) const noexcept
			{
				if (IsWindow(hWnd))
				{
					Util::WarnAssertTrue(DestroyWindow(hWnd), "DestroyWindow");
				}
			}
		};

		using WindowHandle = std::unique_ptr<HWND__, WindowDestroyer>;

		class ClassInfoStore
		{
		public:
			static std::string Register(int const icon, int const menu, WNDPROC const proc)
			{
				static_cast<void>(icon);
				static_cast<void>(menu);
				// hash+more
				return Formatter::Format("GTL:{0}", static_cast<void *>(proc)); // NOLINT(clang-diagnostic-microsoft-cast)
			}
		};

		inline std::string Register(int const icon, int const menu, WNDPROC const proc)
		{
			std::wstring const className = Cvt::A2W(ClassInfoStore::Register(icon, menu, proc));
			auto * instance = Instance();

			WNDCLASSEXW wc {};
			BOOL const exists = GetClassInfoExW(instance, className.c_str(), &wc);
			if (exists == 0)
			{
				Util::AssertTrue(GetLastError() == ERROR_CLASS_DOES_NOT_EXIST, "GetClassInfoExW");

				HICON const i = icon == 0 ? nullptr : LoadIconW(instance, MakeIntResource(icon));

				wc = {sizeof(WNDCLASSEXW),
							HRedraw | VRedraw,
							proc,
							0,
							0,
							instance,
							i,
							LoadCursorW(nullptr, IDC_ARROW), // NOLINT bad macro
							Util::Detail::WindowsCast<HBRUSH>(size_t {COLOR_WINDOW} + 1),
							MakeIntResource(menu),
							className.c_str(),
							{}};

				Util::AssertTrue(RegisterClassExW(&wc) != 0, "RegisterClassExW");
			}
			return Cvt::W2A(className);
		}

		inline void AssociateHandle(Window * value, HWND const handle)
		{
			SetLastError(ERROR_SUCCESS); // SetWindowLongPtr does not set last error on success
			auto const ret = SetWindowLongPtrW(handle, GWLP_USERDATA, Util::Detail::WindowsCast<LONG_PTR>(value));
			Util::AssertTrue(ret != 0 || GetLastError() == ERROR_SUCCESS, "SetWindowLongPtr");
		}

		inline Window * FromHandle(HWND const hWnd)
		{
			return Util::Detail::WindowsCast<Window *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		}

		inline WindowHandle Create(ULONG const style, int const icon, int const menu, std::string const & title, WNDPROC const proc, Window * const param)
		{
			std::string const className = Register(icon, menu, proc);
			WindowHandle handle(CreateWindowExW(0, Cvt::A2W(className).c_str(), Cvt::A2W(title).c_str(), style, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr,
																					nullptr, Instance(), param));
			Util::AssertTrue(!!handle, "CreateWindowExW");
			AssociateHandle(param, handle.get());

#ifdef GLIB_DEBUG_MESSAGES
			MessageDebug::Write("Window create", handle.get(), param);
#endif

			return handle;
		}

		inline HACCEL LoadAccel(int const id)
		{
			HACCEL const accel = LoadAcceleratorsW(Instance(), MakeIntResource(id));
			Util::AssertTrue(accel != nullptr, "LoadAcceleratorsW");
			return accel;
		}
	}

	enum class CloseResult
	{
		Allow,
		Prevent
	};

	class Window
	{
		Detail::WindowHandle handle;
		HACCEL accel {};

	public:
		Window(int const icon, int const menu, int const accel, std::string const & title)
			: handle {Detail::Create(Detail::OverlappedWindow, icon, menu, title, WindowProc, this)}
			, accel {accel != 0 ? Detail::LoadAccel(accel) : nullptr}
		{}

		Window(Window const &) = delete;
		Window(Window &&) = delete;
		Window & operator=(Window const &) = delete;
		Window & operator=(Window &&) = delete;

	protected:
		virtual ~Window() = default;

		[[nodiscard]] HWND Handle() const
		{
			return handle.get();
		}

		static HINSTANCE Instance()
		{
			return Detail::Instance();
		}

		[[nodiscard]] LRESULT Send(UINT const msg, WPARAM const wParam = {}, LPARAM const lParam = {}) const
		{
			return SendMessageW(Handle(), msg, wParam, lParam);
		}

	public:
		[[nodiscard]] int PumpMessages() const
		{
			MSG msg = {};
			while (GetMessageW(&msg, nullptr, 0, 0) != FALSE) // returns -1 on error?
			{
				auto const ret = TranslateAcceleratorW(handle.get(), accel, &msg); // no error if hAccel is null
				if (ret == 0)
				{
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}
			}
			return static_cast<int>(msg.wParam);
		}

		[[nodiscard]] Size ClientSize() const
		{
			RECT rc;
			Util::AssertTrue(GetClientRect(handle.get(), &rc), "GetClientRect");
			return {rc.right - rc.left, rc.bottom - rc.top};
		}

		[[nodiscard]] bool Show(int const cmd) const
		{
			return ShowWindow(handle.get(), cmd) != FALSE;
		}

		[[nodiscard]] Painter GetPainter() const
		{
			PAINTSTRUCT ps {};
			auto * dc = BeginPaint(handle.get(), &ps);
			return Painter {PaintInfo {ps, handle.get(), dc}};
		}

		void Close() const
		{
			static_cast<void>(Send(WM_CLOSE));
		}

		void Destroy() const
		{
			DestroyWindow(handle.get());
		}

#pragma push_macro("MessageBox")
#undef MessageBox
		[[nodiscard]] int MessageBox(std::string const & message, std::string const & caption, UINT const type = MB_OK) const
		{
			int const result = MessageBoxW(handle.get(), Cvt::A2W(message).c_str(), Cvt::A2W(caption).c_str(), type);
			Util::AssertTrue(result != 0, "MessageBoxW");
			return result;
		}
#pragma pop_macro("MessageBox")

		template <typename R, typename P>
		void SetTimer(std::chrono::duration<R, P> const & interval, UINT_PTR id = 1) const
		{
			auto const count = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
			auto const milliseconds = GLib::Util::CheckedCast<ULONG>(count);
			UINT_PTR const timerResult = ::SetTimer(handle.get(), id, milliseconds, nullptr);
			Util::AssertTrue(timerResult != 0, "SetTimer");
		}

		void KillTimer(UINT_PTR const id = 1) const
		{
			Util::AssertTrue(::KillTimer(handle.get(), id), "KillTimer");
		}

		void Invalidate(bool const erase) const
		{
			Util::AssertTrue(InvalidateRect(Handle(), nullptr, erase ? TRUE : FALSE), "InvalidateRect");
		}

		static void SetHandled(bool const newValue = true)
		{
			SetHandled(newValue, true);
		}

		static bool GetHandled()
		{
			return SetHandled(false, false);
		}

	private:
		static bool SetHandled(bool const newValue, bool const exchange)
		{
			static thread_local bool handled;
			return exchange ? std::exchange(handled, newValue) : handled;
		}

		virtual void OnSize(Size const & /**/) noexcept {}
		virtual void OnGetMinMaxInfo(MINMAXINFO & /**/) noexcept {}
		virtual void OnCommand(int /*command*/) noexcept {}
		virtual CloseResult OnClose() noexcept
		{
			return CloseResult::Allow;
		}
		virtual void OnDestroy() noexcept {}
		virtual void OnNcDestroy() noexcept {}
		virtual void OnUser(WPARAM /*w*/, LPARAM /*p*/) noexcept {}
		virtual void OnNotify(NMHDR const & /*hdr*/) noexcept {}
		virtual void OnChar(int /*char*/) noexcept {}
		virtual void OnKeyDown(int /*key*/) noexcept {}
		virtual void OnLeftButtonDown(Point const & /**/) noexcept {}
		virtual void OnLeftButtonUp(Point const & /**/) noexcept {}
		virtual void OnContextMenu(Point const & /**/) noexcept {}
		virtual void OnMouseMove(Point const & /**/) noexcept {}
		virtual void OnMouseWheel(Point const & /**/, int /*delta*/) noexcept {}
		virtual void OnCaptureChanged() noexcept {}
		virtual void OnGetDlgCode(int /*key*/, MSG const & /**/, LRESULT & /**/) {}
		virtual void OnPaint() noexcept {}
		virtual void OnTimer() noexcept {}
		virtual void OnEraseBackground() noexcept {}

		void OnMessage(UINT const message, WPARAM const wParam, LPARAM const lParam, LRESULT & result) noexcept
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
					return OnGetMinMaxInfo(*Util::Detail::WindowsCast<MINMAXINFO *>(lParam));
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
					OnNcDestroy();
					static_cast<void>(handle.release());
					return;
				}

				case WM_USER:
				{
					return OnUser(wParam, lParam);
				}

				case WM_NOTIFY:
				{
					return OnNotify(*Util::Detail::WindowsCast<NMHDR const *>(lParam));
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
					// WORD keys = GET_KEYSTATE_WPARAM(wParam);
					return OnMouseWheel(Detail::PointFromParam(lParam), Detail::WheelData(wParam));
				}

				case WM_CAPTURECHANGED:
				{
					return OnCaptureChanged();
				}

				case WM_GETDLGCODE:
				{
					return OnGetDlgCode(static_cast<int>(wParam), *Util::Detail::WindowsCast<MSG *>(lParam), result);
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

		static LRESULT APIENTRY WindowProc(HWND const hWnd, UINT const message, WPARAM const wParam, LPARAM const lParam) noexcept
		{
			LRESULT result = 0;
			Window * window = Detail::FromHandle(hWnd);
			bool handled {};
			if (window != nullptr)
			{
				auto const oldValue = SetHandled(false, true);
				window->OnMessage(message, wParam, lParam, result);
				handled = SetHandled(oldValue, true);
			}

			if (!handled)
			{
				result = DefWindowProc(hWnd, message, wParam, lParam);
			}

#ifdef GLIB_DEBUG_MESSAGES
			MessageDebug::Write(Detail::FromHandle(hWnd), hWnd, message, wParam, lParam, result);
#endif

			return result;
		}
	};
}
