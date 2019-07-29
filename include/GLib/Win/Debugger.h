#pragma once

#include <GLib/Win/FileSystem.h>
#include <GLib/Win/Process.h>
#include <GLib/Win/Symbols.h>
#include <GLib/Win/DebugStream.h>

#include <GLib/scope.h>
#include <GLib/split.h>

#include <optional>

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			inline const EXCEPTION_DEBUG_INFO & Exception(const DEBUG_EVENT & event) { return event.u.Exception; } // NOLINT(cppcoreguidelines-pro-type-union-access)
			inline const CREATE_THREAD_DEBUG_INFO & CreateThread(const DEBUG_EVENT & event) { return event.u.CreateThread; } // NOLINT(cppcoreguidelines-pro-type-union-access)
			inline const CREATE_PROCESS_DEBUG_INFO & CreateProcessInfo(const DEBUG_EVENT & event) { return event.u.CreateProcessInfo; } // NOLINT(cppcoreguidelines-pro-type-union-access)
			inline const EXIT_THREAD_DEBUG_INFO & ExitThread(const DEBUG_EVENT & event) { return event.u.ExitThread; } // NOLINT(cppcoreguidelines-pro-type-union-access)
			inline const EXIT_PROCESS_DEBUG_INFO & ExitProcess(const DEBUG_EVENT & event) { return event.u.ExitProcess; } // NOLINT(cppcoreguidelines-pro-type-union-access)
			inline const LOAD_DLL_DEBUG_INFO & LoadDll(const DEBUG_EVENT & event) { return event.u.LoadDll; } // NOLINT(cppcoreguidelines-pro-type-union-access)
			inline const UNLOAD_DLL_DEBUG_INFO & UnloadDll(const DEBUG_EVENT & event) { return event.u.UnloadDll; } // NOLINT(cppcoreguidelines-pro-type-union-access)
			inline const OUTPUT_DEBUG_STRING_INFO & DebugString(const DEBUG_EVENT & event) { return event.u.DebugString; } // NOLINT(cppcoreguidelines-pro-type-union-access)

			inline uint64_t ConvertAddress(const void * address)
			{
				return reinterpret_cast<uint64_t>(address); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
			}
		}

		class Debugger
		{
			Symbols::Engine symbols;
			std::map<std::string, std::string> driveMap;
			Process mainProcess;
			DWORD const debugProcessId;
			std::optional<DWORD> exitCode;
			std::string pendingDebugOut;

		public:
			Debugger(const std::string & path)
				: driveMap(FileSystem::DriveMap())
				, mainProcess(path, DEBUG_ONLY_THIS_PROCESS) // option for DEBUG_PROCESS
				, debugProcessId(mainProcess.Id())
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
				return exitCode.value();
			}

			bool ProcessEvents(DWORD timeout)
			{
				DEBUG_EVENT debugEvent {};
				BOOL const result = ::WaitForDebugEventEx(&debugEvent, timeout);
				Util::WarnAssertTrue(result==TRUE || ::GetLastError()==ERROR_SEM_TIMEOUT, "WaitForDebugEventEx");
				if (result == FALSE)
				{
					return !exitCode.has_value();
				}

				DWORD const processId = debugEvent.dwProcessId;
				DWORD const threadId = debugEvent.dwThreadId;
				DWORD continueStatus = DBG_CONTINUE;

				switch (debugEvent.dwDebugEventCode)
				{
					case CREATE_PROCESS_DEBUG_EVENT:
					{
						auto pi = Detail::CreateProcessInfo(debugEvent);
						SCOPE(_, [&] { ::CloseHandle(pi.hFile); });
						OnCreateProcess(processId, threadId, pi);
						break;
					}

					case EXIT_PROCESS_DEBUG_EVENT:
					{
						OnExitProcess(processId, threadId, Detail::ExitProcess(debugEvent));
						break;
					}

					case CREATE_THREAD_DEBUG_EVENT:
					{
						OnCreateThread(processId, threadId, Detail::CreateThread(debugEvent));
						break;
					}

					case EXIT_THREAD_DEBUG_EVENT:
					{
						OnExitThread(processId, threadId, Detail::ExitThread(debugEvent));
						break;
					}

					case LOAD_DLL_DEBUG_EVENT:
					{
						SCOPE(_, [&] { ::CloseHandle(Detail::LoadDll(debugEvent).hFile); });
						OnLoadDll(processId, threadId, Detail::LoadDll(debugEvent));
						break;
					}

					case UNLOAD_DLL_DEBUG_EVENT:
					{
						OnUnloadDll(processId, threadId, Detail::UnloadDll(debugEvent));
						break;
					}

					case EXCEPTION_DEBUG_EVENT:
					{
						continueStatus = OnException(processId, threadId, Detail::Exception(debugEvent));
						break;
					}

					case OUTPUT_DEBUG_STRING_EVENT:
					{
						OnDebugString(processId, threadId, Detail::DebugString(debugEvent));
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
						Debug::Stream() << "GDB Unknown Event: " << debugEvent.dwDebugEventCode << std::endl;
						break;
					}
				}

				Util::AssertTrue(::ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus), "ContinueDebugEvent failed");
				return true;
			}

		protected:
			virtual void OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(threadId);

				std::string const logicalName = FileSystem::PathOfFileHandle(info.hFile, VOLUME_NAME_NT);
				std::string const name = FileSystem::NormalisePath(logicalName, driveMap);

				// when using DEBUG_ONLY_THIS_PROCESS only get called here for main executable
				const Symbols::SymProcess & process = symbols.AddProcess(processId, info.hProcess, Detail::ConvertAddress(info.lpBaseOfImage), info.hFile, name);

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

				Debug::Stream() << "GDB LoadDll: " << name << " " << info.lpBaseOfDll << std::endl;
			}

			virtual void OnUnloadDll(DWORD processId, DWORD threadId, const UNLOAD_DLL_DEBUG_INFO & info) const
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				Debug::Stream() << "GDB UnloadDll: " << info.lpBaseOfDll << std::endl;
			}

			virtual void OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				Debug::Stream() << "GDB CreateThread: " << info.hThread << std::endl;
			}

			virtual void OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				Debug::Stream() << "GDB ThreadExit code: " << info.dwExitCode << std::endl;
			}

			virtual DWORD OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);
				UNREFERENCED_PARAMETER(info);
				return DBG_CONTINUE;
			}

			virtual void OnDebugString(DWORD processId, DWORD threadId, const OUTPUT_DEBUG_STRING_INFO & info)
			{
				UNREFERENCED_PARAMETER(processId);
				UNREFERENCED_PARAMETER(threadId);

				//size_t bytes = info.nDebugStringLength;
				//auto buffer = std::make_unique<unsigned char[]>(bytes);
				//mainProcess.ReadMemory(Detail::ConvertAddress(info.lpDebugStringData), buffer.get(), bytes);
				auto address = Detail::ConvertAddress(info.lpDebugStringData);

				std::string message;
				if (info.fUnicode != 0)
				{
					size_t size = info.nDebugStringLength / 2 - 1;
					message =  Cvt::w2a(std::wstring{ mainProcess.ReadMemory<wchar_t>(address, size).get(), size });
				}
				else
				{
					size_t size = info.nDebugStringLength - 1;
					message =  std::string{ mainProcess.ReadMemory<char>(address, size).get(), size };
				}

				message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
				GLib::Util::Splitter splitter(message, "\n");
				for (auto it = splitter.begin(), end = splitter.end(); it != end;)
				{
					std::string s = *it;
					if (++it != end)
					{
						Debug::Stream() << "GDB: " << pendingDebugOut << s << std::endl;
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