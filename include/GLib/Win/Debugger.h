#pragma once

#include <GLib/Win/FileSystem.h>
#include <GLib/Win/Process.h>
#include <GLib/Win/Symbols.h>

#include <GLib/scope.h>
#include <GLib/split.h>

namespace GLib
{
	namespace Win
	{
		class Debugger
		{
			Symbols::Engine symbols;
			std::map<std::string, std::string> driveMap;
			Process mainProcess;
			DWORD const debugProcessId;
			DWORD exitCode;
			std::string pendingDebugOut;

		public:
			Debugger(const std::string & path)
				: driveMap(FileSystem::DriveMap())
				, mainProcess(path, DEBUG_ONLY_THIS_PROCESS) // option for DEBUG_PROCESS
				, debugProcessId(mainProcess.Id())
				, exitCode()
			{}

			Debugger(const Debugger&) = delete;
			Debugger(Debugger&&) = delete;
			Debugger& operator=(const Debugger&) = delete;
			Debugger& operator=(Debugger&&) = delete;
			virtual ~Debugger() = default;

			const Symbols::Engine & Symbols() const
			{
				return symbols;
			}

			DWORD ExitCode() const 
			{
				return exitCode;
			}

			bool ProcessEvents(DWORD timeout)
			{
				DEBUG_EVENT debugEvent {};
				BOOL const result = ::WaitForDebugEventEx(&debugEvent, timeout);
				Util::WarnAssertTrue(result, "WaitForDebugEventEx");
				if (!result)
				{
					return false;
				}

				DWORD const processId = debugEvent.dwProcessId;
				DWORD const threadId = debugEvent.dwThreadId;

				switch (debugEvent.dwDebugEventCode)
				{
					case CREATE_PROCESS_DEBUG_EVENT:
					{
						SCOPE(_, [&] { ::CloseHandle(debugEvent.u.CreateProcessInfo.hFile); });
						OnCreateProcess(processId, threadId, debugEvent.u.CreateProcessInfo);
						break;
					}

					case EXIT_PROCESS_DEBUG_EVENT:
					{
						OnExitProcess(processId, threadId, debugEvent.u.ExitProcess);
						break;
					}

					case CREATE_THREAD_DEBUG_EVENT:
					{
						OnCreateThread(processId, threadId, debugEvent.u.CreateThread);
						break;
					}

					case EXIT_THREAD_DEBUG_EVENT:
					{
						OnExitThread(processId, threadId, debugEvent.u.ExitThread);
						break;
					}

					case LOAD_DLL_DEBUG_EVENT:
					{
						SCOPE(_, [&] { ::CloseHandle(debugEvent.u.LoadDll.hFile); });
						OnLoadDll(processId, threadId, debugEvent.u.LoadDll);
						break;
					}

					case UNLOAD_DLL_DEBUG_EVENT:
					{
						OnUnloadDll(processId, threadId, debugEvent.u.UnloadDll);
						break;
					}

					case EXCEPTION_DEBUG_EVENT:
					{
						OnException(processId, threadId, debugEvent.u.Exception);
						break;
					}

					case OUTPUT_DEBUG_STRING_EVENT:
					{
						OnDebugString(processId, threadId, debugEvent.u.DebugString);
						break;
					}

					case RIP_EVENT:
					{
						break;
					}

					case 0:
					{
						// timeout
						break;
					}

					default:
					{
						Debug::Detail::DebugStream() << "GDB Unknown Event: " << debugEvent.dwDebugEventCode << std::endl;
						break;
					}
				}

				Util::AssertTrue(::ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE), "ContinueDebugEvent failed");
				return true;
			}

		protected:
			virtual void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(threadId);

				std::string const logicalName = FileSystem::PathOfFileHandle(info.hFile, VOLUME_NAME_NT);
				std::string const name = FileSystem::NormalisePath(logicalName, driveMap);

				// when using DEBUG_ONLY_THIS_PROCESS only get called here for main executable
				const Symbols::SymProcess & process = symbols.AddProcess(processId, info.hProcess, info.lpBaseOfImage, info.hFile, name);

				IMAGE_DOS_HEADER header {};
				process.ReadMemory(0, &header, sizeof(header));
				if (header.e_magic != IMAGE_DOS_SIGNATURE)
				{
					throw std::runtime_error("Unknown image header");
				}

				IMAGE_NT_HEADERS32 headers32 {};
				process.ReadMemory(header.e_lfanew, &headers32, sizeof(headers32));

				switch (headers32.FileHeader.Machine)
				{
					case IMAGE_FILE_MACHINE_I386:
						On32BitProcess(headers32);
						break;

					case IMAGE_FILE_MACHINE_AMD64:
					{
						IMAGE_NT_HEADERS64 headers64 {};
						process.ReadMemory(header.e_lfanew, &headers64, sizeof(headers64));
						On64BitProcess(headers64);
						break;
					}
					default:
						throw std::runtime_error("Unknown machine type");
				}
			}

			virtual void On32BitProcess(const IMAGE_NT_HEADERS32 & headers)
			{
				UNREFERENCED_PARAMETER(headers);
			}

			virtual void On64BitProcess(const IMAGE_NT_HEADERS64 & headers)
			{
				UNREFERENCED_PARAMETER(headers);
			}

			virtual void OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(threadId);

				if (processId == debugProcessId)
				{
					exitCode = info.dwExitCode;
				}

				symbols.RemoveProcess(processId);
			}

			virtual void OnLoadDll(DWORD processId, DWORD threadId, const LOAD_DLL_DEBUG_INFO & info) const
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				std::string const logicalName = FileSystem::PathOfFileHandle(info.hFile, VOLUME_NAME_NT);
				std::string const name = FileSystem::NormalisePath(logicalName, driveMap);

				Debug::Detail::DebugStream() << "GDB LoadDll: " << name << " " << info.lpBaseOfDll << std::endl;
			}

			virtual void OnUnloadDll(DWORD processId, DWORD threadId, const UNLOAD_DLL_DEBUG_INFO & info) const
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				Debug::Detail::DebugStream() << "GDB UnloadDll: " << info.lpBaseOfDll << std::endl;
			}

			virtual void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				Debug::Detail::DebugStream() << "GDB CreateThread: " << info.hThread << std::endl;
			}

			virtual void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				Debug::Detail::DebugStream() << "GDB ThreadExit code: " << info.dwExitCode << std::endl;
			}

			virtual void OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				Debug::Detail::DebugStream() << "GDB Exception: " << std::hex << info.ExceptionRecord.ExceptionCode << std::dec << std::endl;
			}

			virtual void OnDebugString(DWORD processId, DWORD threadId, const OUTPUT_DEBUG_STRING_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				auto buffer = std::make_unique<unsigned char[]>(info.nDebugStringLength);
				mainProcess.ReadMemory(info.lpDebugStringData, buffer.get(), info.nDebugStringLength);

				std::string message;
				if (info.fUnicode)
				{
					message = Cvt::w2a(std::wstring(reinterpret_cast<const wchar_t*>(buffer.get()), info.nDebugStringLength / 2 - 1));
				}
				else
				{
					message = std::string(reinterpret_cast<const char *>(buffer.get()), info.nDebugStringLength - 1);
				}

				message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
				GLib::Util::Splitter splitter(message, "\n");
				for (auto it = splitter.begin(), end = splitter.end(); it != end;)
				{
					std::string s = *it;
					if (++it != end)
					{
						Debug::Detail::DebugStream() << "GDB: " << pendingDebugOut << s << std::endl;
						pendingDebugOut.clear();
					}
					else
					{
						pendingDebugOut += s;
					}
				}
			}
		};
	}
}