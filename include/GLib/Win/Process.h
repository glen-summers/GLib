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
		inline const void * ToAddress(uint64_t value)
		{
			return Util::Detail::WindowsCast<const void *>(value);
		}

		inline void * ToPseudoWritableAddress(uint64_t value)
		{
			return Util::Detail::WindowsCast<void *>(value);
		}

		inline bool Terminate(HANDLE process, UINT terminationExitCode) noexcept
		{
			DWORD exitCode = 0;
			BOOL result = GetExitCodeProcess(process, &exitCode);
			bool stillRunning = result != FALSE && exitCode == STILL_ACTIVE;
			if (stillRunning)
			{
				Util::WarnAssertTrue(TerminateProcess(process, terminationExitCode), "TerminateProcess");
			}
			return stillRunning;
		}

		template <UINT ExitCode>
		struct Terminator
		{
			void operator()(HANDLE process) const noexcept
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
		int const threadId;

	public:
		static std::string CurrentPath()
		{
			return FileSystem::PathOfModule(nullptr);
		}

		static int CurrentId()
		{
			return GetCurrentProcessId();
		}

		static HMODULE CurrentModule()
		{
			return Util::Detail::WindowsCast<HMODULE>(&__ImageBase);
		}

		Process(Handle handle)
			: p(std::move(handle))
			, threadId(GetThreadId(p.get()))
		{}

		// creation flags  DETACHED_PROCESS?
		Process(const std::string & app, const std::string & cmd = {}, DWORD creationFlags = {}, WORD show = {}, const std::string & desktop = {})
			: Process(Create(app, cmd, creationFlags, show, desktop))
		{}

		// returns immediately for successful start but failed later? e.g. on non existent desktop
		template <typename R, typename P>
		void WaitForInputIdle(const std::chrono::duration<R, P> & duration) const
		{
			auto count = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			DWORD timeoutMilliseconds = GLib::Util::CheckedCast<DWORD>(count);
			CheckWaitResult(::WaitForInputIdle(p.get(), timeoutMilliseconds));
		}

		[[nodiscard]] const Handle & Handle() const
		{
			return p;
		}

		void Terminate(unsigned int exitCode = 1) const noexcept
		{
			Detail::Terminate(p.get(), exitCode);
		}

		template <UINT ExitCode = 1>
		Detail::TerminatorHolder<ExitCode> ScopedTerminator() const
		{
			return Detail::TerminatorHolder<ExitCode>(p.get());
		}

		[[nodiscard]] DWORD Id() const
		{
			DWORD pid = GetProcessId(p.get());
			Util::AssertTrue(pid != 0, "GetProcessId");
			return pid;
		}

		// no throw on timeout versions? just return bool
		void WaitForExit() const
		{
			Wait(p.get(), INFINITE);
		}

		template <typename R, typename P>
		void WaitForExit(const std::chrono::duration<R, P> & duration) const
		{
			auto ms = GLib::Util::CheckedCast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
			Wait(p.get(), ms);
		}

		[[nodiscard]] DWORD ExitCode() const
		{
			DWORD exitCode = 0;
			BOOL win32Result = GetExitCodeProcess(p.get(), &exitCode);
			Util::AssertTrue(win32Result, "GetExitCodeProcess");
			return exitCode;
		}

		[[nodiscard]] bool IsRunning() const
		{
			return ExitCode() == STILL_ACTIVE;
		}

		void ReadMemory(uint64_t address, void * buffer, size_t size) const
		{
			BOOL result = ReadProcessMemory(p.get(), Detail::ToAddress(address), buffer, size, nullptr);
			Util::AssertTrue(result, "ReadProcessMemory");
		}

		template <typename T>
		void ReadMemory(uint64_t address, void * buffer, size_t size) const
		{
			BOOL result = ReadProcessMemory(p.get(), Detail::ToAddress(address), buffer, size * sizeof(T), nullptr);
			Util::AssertTrue(result, "ReadProcessMemory");
		}

		void WriteMemory(uint64_t address, const void * buffer, size_t size) const
		{
			BOOL result = WriteProcessMemory(p.get(), Detail::ToPseudoWritableAddress(address), buffer, size, nullptr);
			Util::AssertTrue(result, "WriteProcessMemory");
		}

	private:
		static Win::Handle Create(const std::string & app, const std::string & cmd, DWORD creationFlags, WORD show, const std::string & desktop)
		{
			return Create(Cvt::A2W(app), Cvt::A2W(app + " " + cmd), creationFlags, show, Cvt::A2W(desktop));
		}

		static Win::Handle Create(const std::wstring & app, const std::wstring & cmd, DWORD creationFlags, WORD show, const std::wstring & desktop)
		{
			STARTUPINFOW sui = {};
			sui.cb = sizeof(STARTUPINFOW);

			std::unique_ptr<wchar_t, void (*)(void *)> cmdCopy {_wcsdup(cmd.c_str()), std::free};
			std::unique_ptr<wchar_t, void (*)(void *)> desktopCopy {_wcsdup(desktop.c_str()), std::free};

			sui.lpDesktop = desktopCopy.get();
			sui.dwFlags = STARTF_USESHOWWINDOW;
			sui.wShowWindow = show;

			PROCESS_INFORMATION pi = {};

			Util::AssertTrue(CreateProcessW(app.c_str(), cmdCopy.get(), nullptr, nullptr, FALSE, creationFlags, nullptr, nullptr, &sui, &pi),
											 "CreateProcessW");
			Win::Handle(pi.hThread).reset();
			return Win::Handle {pi.hProcess};
		}

		static void Wait(HANDLE h, DWORD timeoutMilliseconds)
		{
			CheckWaitResult(WaitForSingleObject(h, timeoutMilliseconds));
		}

		static void CheckWaitResult(DWORD result)
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