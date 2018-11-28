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
				inline void Terminate(HANDLE process, UINT exitCode = 1)
				{
					//if (exitCode == STILL_ACTIVE)?
					Util::WarnAssertTrue(::TerminateProcess(process, exitCode), "TerminateProcess");
				}

				struct Terminator
				{
					void operator()(HANDLE process) const noexcept
					{
						Terminate(process);
					}
				};
				typedef std::unique_ptr<void, Terminator> TerminatorHolder;

				// move?
				inline std::string EnvironmentVariable(const std::string & name)
				{
					std::wstring w = Cvt::a2w(name);
					GLib::Util::StackOrHeap<wchar_t, 256> s;
					size_t size = ::GetEnvironmentVariableW(w.c_str(), s.Get(), static_cast<DWORD>(s.size()));
					if (size >= s.size())
					{
						s.EnsureSize(size);
						size = ::GetEnvironmentVariableW(w.c_str(), s.Get(), static_cast<DWORD>(s.size()));
					}
					return size == 0 ? "" : Cvt::w2a(s.Get());
				}
			}

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

				const Handle & Handle() const
				{
					return p;
				}

				void Terminate(unsigned int exitCode = 1) const
				{
					Detail::Terminate(p.get(), exitCode);
				}

				Detail::TerminatorHolder ScopedTerminator() const
				{
					return Detail::TerminatorHolder(p.get());
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

				void ReadMemory(const void * address, void * buffer, size_t size) const
				{
					BOOL result = ::ReadProcessMemory(p.get(), address, buffer, size, nullptr);
					Util::AssertTrue(result, "ReadProcessMemory");
				}

			private:
				static Win::Handle Create(const std::string & app, DWORD creationFlags, const std::string & desktop)
				{
					return Create(Cvt::a2w(app), creationFlags, Cvt::a2w(desktop));
				}

				static Win::Handle Create(const std::wstring & app, DWORD creationFlags, const std::wstring & desktop)
				{
					// add standard handles? verify
					STARTUPINFOW sui = {};
					sui.cb = sizeof(STARTUPINFOW);
					sui.lpDesktop = desktop.empty() ? nullptr : const_cast<LPWSTR>(desktop.c_str());
					PROCESS_INFORMATION pi = {};
					Util::AssertTrue(::CreateProcessW(app.c_str(), nullptr, nullptr, nullptr, FALSE, creationFlags,
						nullptr, nullptr, &sui, &pi), "CreateProcess");
					Win::Handle p(pi.hProcess);
					Win::Handle t(pi.hThread);
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