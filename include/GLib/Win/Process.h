#pragma once

#include "ErrorCheck.h"
#include "Handle.h"
#include "Module.h"

#include <GLib/checked_cast.h>

#include <filesystem>

namespace GLib
{
	namespace Win
	{
		namespace Process
		{
			namespace Detail
			{
				struct Terminator
				{
					void operator()(HANDLE process) const noexcept
					{
						//if (exitCode == STILL_ACTIVE)?
						Util::WarnAssertTrue(::TerminateProcess(process, 1), "TerminateProcess");
					}
				};
			}

			typedef std::unique_ptr<void, Detail::Terminator> Handle;

			inline std::filesystem::path CurrentProcessPath()
			{
				return Module::FileName(nullptr);
			}

			inline int CurrentProcessId()
			{
				return ::GetCurrentProcessId();
			}

			class Process
			{
				Handle p;
				int const threadId;

			public:
				Process(const std::string & app, DWORD creationFlags = {}, const std::string & desktop = {})
					: p(Create(app, creationFlags, desktop))
					, threadId(::GetThreadId(p.get()))
				{}

				// returns immediately for successful start but failed later? e.g. on non existent desktop
				template<typename R, typename P>
				void WaitForInputIdle(const std::chrono::duration<R, P> & duration) const
				{
					auto count = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
					DWORD timeoutMilliseconds = GLib::Util::checked_cast<DWORD>(count);
					CheckWaitResult(::WaitForInputIdle(p.get(), timeoutMilliseconds));
				}

				void Terminate(unsigned int exitCode = 1) const
				{
					//assert(exitCode == STILL_ACTIVE)
					Util::AssertTrue(::TerminateProcess(p.get(), exitCode), "Terminate");
				}

				Handle ScopedTerminator() const
				{
					return Handle(p.get());
				}

				DWORD Id() const
				{
					return ::GetProcessId(p.get());
				}

				// no throw on timeout versions? just return bool
				void WaitForExit() const
				{
					Wait(p.get(), INFINITE);
				}

				template<typename R, typename P>
				void WaitForExit(const std::chrono::duration<R, P> & duration) const
				{
					auto ms =GLib::Util::checked_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
					Wait(p.get(), ms);
				}

				DWORD ExitCode() const
				{
					DWORD exitCode;
					BOOL win32Result = ::GetExitCodeProcess(p.get(), &exitCode);
					Util::AssertTrue(win32Result, "GetExitCodeProcess");
					return exitCode;
				}

				bool IsRunning() const
				{
					return ExitCode() == STILL_ACTIVE;
				}

			private:
				static Handle Create(const std::string & app, DWORD creationFlags, const std::string & desktop)
				{
					return Create(Cvt::a2w(app), creationFlags, Cvt::a2w(desktop));
				}

				static Handle Create(const std::wstring & app, DWORD creationFlags, const std::wstring & desktop)
				{
					STARTUPINFOW sui = {};
					sui.cb = sizeof(STARTUPINFOW);
					sui.lpDesktop = desktop.empty() ? nullptr : const_cast<LPWSTR>(desktop.c_str());
					PROCESS_INFORMATION pi = {};
					Util::AssertTrue(::CreateProcessW(app.c_str(), nullptr, nullptr, nullptr, FALSE, creationFlags,
						nullptr, nullptr, &sui, &pi), "CreateProcess");
					Handle p(pi.hProcess);
					Handle t(pi.hThread);
					//DWORD pid = pi.dwProcessId;
					return p;
				}

				static void Wait(HANDLE h, DWORD timeoutMilliseconds)
				{
					CheckWaitResult(::WaitForSingleObject(h, timeoutMilliseconds));
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
							Util::Detail::Throw("CheckWaitResult", ::GetLastError());
						default:
							throw std::runtime_error("Unexpected result");
					}
				}
			};
		}
	}
}