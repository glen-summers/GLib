#pragma once

#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/FileSystem.h>
#include <GLib/Win/Handle.h>

#include <GLib/CheckedCast.h>

#include <chrono>

namespace GLib::Win
{
	namespace Detail
	{
		inline void const * ToAddress(uint64_t const value)
		{
			return Util::Detail::WindowsCast<void const *>(value);
		}

		inline void * ToPseudoWritableAddress(uint64_t const value)
		{
			return Util::Detail::WindowsCast<void *>(value);
		}

		inline bool Terminate(HandleBase * const process, UINT const terminationExitCode) noexcept
		{
			ULONG exitCode = 0;
			BOOL const result = GetExitCodeProcess(process, &exitCode);
			bool const stillRunning = result != FALSE && exitCode == STILL_ACTIVE;
			if (stillRunning)
			{
				Util::WarnAssertTrue(TerminateProcess(process, terminationExitCode), "TerminateProcess");
			}
			return stillRunning;
		}

		template <UINT ExitCode>
		struct Terminator
		{
			void operator()(HandleBase * const process) const noexcept
			{
				Terminate(process, ExitCode);
			}
		};

		template <UINT ExitCode>
		using TerminatorHolder = std::unique_ptr<void, Terminator<ExitCode>>;
	}

	class Process
	{
		Handle p;
		ULONG const threadId;

	public:
		static std::string CurrentPath()
		{
			return FileSystem::PathOfModule(nullptr);
		}

		static ULONG CurrentId()
		{
			return GetCurrentProcessId();
		}

		static HMODULE CurrentModule()
		{
			return Util::Detail::WindowsCast<HMODULE>(&__ImageBase);
		}

		explicit Process(Handle handle)
			: p(std::move(handle))
			, threadId(GetThreadId(p.get()))
		{}

		// creation flags  DETACHED_PROCESS?
		explicit Process(std::string const & app, std::string const & cmd = {}, ULONG const creationFlags = {}, WORD const show = {},
										 std::string const & desktop = {})
			: Process(Create(app, cmd, creationFlags, show, desktop))
		{}

		// returns immediately for successful start but failed later? e.g. on non existent desktop
		template <typename R, typename P>
		void WaitForInputIdle(std::chrono::duration<R, P> const & duration) const
		{
			auto const count = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			auto const timeoutMilliseconds = GLib::Util::CheckedCast<ULONG>(count);
			CheckWaitResult(::WaitForInputIdle(p.get(), timeoutMilliseconds));
		}

		[[nodiscard]] Handle const & Handle() const
		{
			return p;
		}

		void Terminate(unsigned int const exitCode = 1) const noexcept
		{
			Detail::Terminate(p.get(), exitCode);
		}

		template <UINT ExitCode = 1>
		Detail::TerminatorHolder<ExitCode> ScopedTerminator() const
		{
			return Detail::TerminatorHolder<ExitCode>(p.get());
		}

		[[nodiscard]] ULONG Id() const
		{
			ULONG const pid = GetProcessId(p.get());
			Util::AssertTrue(pid != 0, "GetProcessId");
			return pid;
		}

		// no throw on timeout versions? just return bool
		void WaitForExit() const
		{
			Wait(p.get(), INFINITE);
		}

		template <typename R, typename P>
		void WaitForExit(std::chrono::duration<R, P> const & duration) const
		{
			auto const ms = GLib::Util::CheckedCast<ULONG>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
			Wait(p.get(), ms);
		}

		[[nodiscard]] ULONG ExitCode() const
		{
			ULONG exitCode = 0;
			BOOL const win32Result = GetExitCodeProcess(p.get(), &exitCode);
			Util::AssertTrue(win32Result, "GetExitCodeProcess");
			return exitCode;
		}

		[[nodiscard]] bool IsRunning() const
		{
			return ExitCode() == STILL_ACTIVE;
		}

		void ReadMemory(uint64_t const address, void * const buffer, size_t const size) const
		{
			BOOL const result = ReadProcessMemory(p.get(), Detail::ToAddress(address), buffer, size, nullptr);
			Util::AssertTrue(result, "ReadProcessMemory");
		}

		template <typename T>
		void ReadMemory(uint64_t const address, void * const buffer, size_t const size) const
		{
			BOOL const result = ReadProcessMemory(p.get(), Detail::ToAddress(address), buffer, size * sizeof(T), nullptr);
			Util::AssertTrue(result, "ReadProcessMemory");
		}

		void WriteMemory(uint64_t const address, void const * const buffer, size_t const size) const
		{
			BOOL const result = WriteProcessMemory(p.get(), Detail::ToPseudoWritableAddress(address), buffer, size, nullptr);
			Util::AssertTrue(result, "WriteProcessMemory");
		}

	private:
		static Win::Handle Create(std::string const & app, std::string const & cmd, ULONG const creationFlags, WORD const show,
															std::string const & desktop)
		{
			return Create(Cvt::A2W(app), Cvt::A2W(app + " " + cmd), creationFlags, show, Cvt::A2W(desktop));
		}

		static Win::Handle Create(std::wstring const & app, std::wstring const & cmd, ULONG const creationFlags, WORD const show,
															std::wstring const & desktop)
		{
			STARTUPINFOW sui = {};
			sui.cb = sizeof(STARTUPINFOW);

			std::unique_ptr<wchar_t, void (*)(void *)> const cmdCopy {_wcsdup(cmd.c_str()), std::free};
			std::unique_ptr<wchar_t, void (*)(void *)> const desktopCopy {_wcsdup(desktop.c_str()), std::free};

			sui.lpDesktop = desktopCopy.get();
			sui.dwFlags = STARTF_USESHOWWINDOW;
			sui.wShowWindow = show;

			PROCESS_INFORMATION info = {};

			Util::AssertTrue(CreateProcessW(app.c_str(), cmdCopy.get(), nullptr, nullptr, FALSE, creationFlags, nullptr, nullptr, &sui, &info),
											 "CreateProcessW");
			Win::Handle(info.hThread).reset();
			return Win::Handle {info.hProcess};
		}

		static void Wait(HandleBase * const handle, ULONG const timeoutMilliseconds)
		{
			CheckWaitResult(WaitForSingleObject(handle, timeoutMilliseconds));
		}

		static void CheckWaitResult(ULONG const result)
		{
			switch (result)
			{
				case ERROR_SUCCESS: // also WAIT_OBJECT_0:
					break;
				case WAIT_TIMEOUT:
					throw std::exception("Timeout waiting for exit");
				case WAIT_FAILED:
					Util::Detail::Throw("CheckWaitResult", GetLastError());
				default:
					throw std::runtime_error("Unexpected result");
			}
		}
	};
}