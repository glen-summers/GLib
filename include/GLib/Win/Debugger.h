#pragma once

#include <GLib/Win/DebugWrite.h>
#include <GLib/Win/FileSystem.h>
#include <GLib/Win/Process.h>
#include <GLib/Win/Symbols.h>

#include <GLib/Split.h>

#include <optional>

namespace GLib::Win
{
	namespace Detail
	{
		// clang-format off
		inline EXCEPTION_DEBUG_INFO      const & Exception        (DEBUG_EVENT const & event) { return event.u.Exception; }         // NOLINT(cppcoreguidelines-pro-type-union-access)
		inline CREATE_THREAD_DEBUG_INFO  const & CreateThread     (DEBUG_EVENT const & event) { return event.u.CreateThread; }      // NOLINT
		inline CREATE_PROCESS_DEBUG_INFO const & CreateProcessInfo(DEBUG_EVENT const & event) { return event.u.CreateProcessInfo; } // NOLINT
		inline EXIT_THREAD_DEBUG_INFO    const & ExitThread       (DEBUG_EVENT const & event) { return event.u.ExitThread; }        // NOLINT
		inline EXIT_PROCESS_DEBUG_INFO   const & ExitProcess      (DEBUG_EVENT const & event) { return event.u.ExitProcess; }       // NOLINT
		inline LOAD_DLL_DEBUG_INFO       const & LoadDll          (DEBUG_EVENT const & event) { return event.u.LoadDll; }           // NOLINT
		inline UNLOAD_DLL_DEBUG_INFO     const & UnloadDll        (DEBUG_EVENT const & event) { return event.u.UnloadDll; }         // NOLINT
		inline OUTPUT_DEBUG_STRING_INFO  const & DebugString      (DEBUG_EVENT const & event) { return event.u.DebugString; }       // NOLINT
		// clang-format off

		inline uint64_t ConvertAddress(void const * address)
		{
			return Util::Detail::WindowsCast<uint64_t>(address);
		}
	}

	class Debugger
	{
		Symbols::Engine symbols;
		std::map<std::string, std::string> driveMap;
		Process mainProcess;
		ULONG const debugProcessId;
		std::optional<ULONG> exitCode;
		std::string pendingDebugOut;

	public:
		Debugger(std::string const & path, bool const debugChildProcesses)
			: driveMap(FileSystem::DriveMap())
			, mainProcess(path, {}, debugChildProcesses ? DEBUG_PROCESS : DEBUG_ONLY_THIS_PROCESS)
			, debugProcessId(mainProcess.Id())
		{}

		Debugger(Debugger const &) = delete;
		Debugger(Debugger &&) = delete;
		Debugger & operator=(Debugger const &) = delete;
		Debugger & operator=(Debugger &&) = delete;
		virtual ~Debugger() = default;

		[[nodiscard]] Symbols::Engine const & Symbols() const
		{
			return symbols;
		}

		[[nodiscard]] ULONG ProcessId() const
		{
			return debugProcessId;
		}

		[[nodiscard]] ULONG ExitCode() const
		{
			return exitCode.has_value() ? *exitCode : throw std::runtime_error{"Unexpected empty value"};
		}

		bool ProcessEvents(ULONG const timeout)
		{
			DEBUG_EVENT debugEvent {};
			BOOL const result = WaitForDebugEventEx(&debugEvent, timeout);
			Util::WarnAssertTrue(result == TRUE || GetLastError() == ERROR_SEM_TIMEOUT, "WaitForDebugEventEx");
			if (result == FALSE)
			{
				return !exitCode.has_value();
			}

			ULONG const processId = debugEvent.dwProcessId;
			ULONG const threadId = debugEvent.dwThreadId;
			ULONG continueStatus = DBG_CONTINUE;

			switch (debugEvent.dwDebugEventCode)
			{
				case CREATE_PROCESS_DEBUG_EVENT:
				{
					auto const pi = Detail::CreateProcessInfo(debugEvent);
					Handle const handle {pi.hFile};
					OnCreateProcess(processId, threadId, pi);
					static_cast<void>(handle);
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
					Handle const handle {Detail::LoadDll(debugEvent).hFile};
					OnLoadDll(processId, threadId, Detail::LoadDll(debugEvent));
					static_cast<void>(handle);
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
					Debug::Stream() << "RIP Event : " << debugEvent.dwDebugEventCode << std::endl;
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

			Util::AssertTrue(ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus), "ContinueDebugEvent");
			return true;
		}

	protected:
		virtual void OnCreateProcess(ULONG const processId, ULONG const threadId, CREATE_PROCESS_DEBUG_INFO const & info)
		{
			static_cast<void>(threadId);

			std::string const logicalName = FileSystem::PathOfFileHandle(info.hFile, VOLUME_NAME_NT);
			std::string const name = FileSystem::NormalisePath(logicalName, driveMap);

			Debug::Write("Attach Process: {0}, Pid: {1}", name, processId);

			Symbols::SymProcess const & process =
				symbols.AddProcess(processId, info.hProcess, Detail::ConvertAddress(info.lpBaseOfImage), info.hFile, name);

			IMAGE_DOS_HEADER header {};
			process.ReadMemory(0, &header, sizeof header);
			if (header.e_magic != IMAGE_DOS_SIGNATURE)
			{
				throw std::runtime_error("Unknown image header");
			}

			IMAGE_NT_HEADERS32 headers32 {};
			process.ReadMemory(header.e_lfanew, &headers32, sizeof headers32);

			switch (headers32.FileHeader.Machine)
			{
				case IMAGE_FILE_MACHINE_I386:
					On32BitProcess(headers32);
					break;

				case IMAGE_FILE_MACHINE_AMD64:
				{
					IMAGE_NT_HEADERS64 headers64 {};
					process.ReadMemory(header.e_lfanew, &headers64, sizeof headers64);
					On64BitProcess(headers64);
					break;
				}
				default:
					throw std::runtime_error("Unknown machine type");
			}
		}

		virtual void On32BitProcess( IMAGE_NT_HEADERS32 const & headers)
		{
			static_cast<void>(headers);
		}

		virtual void On64BitProcess(IMAGE_NT_HEADERS64 const & headers)
		{
			static_cast<void>(headers);
		}

		virtual void OnExitProcess(ULONG const processId, ULONG const threadId, EXIT_PROCESS_DEBUG_INFO const & info)
		{
			static_cast<void>(threadId);

			if (processId == debugProcessId)
			{
				exitCode = info.dwExitCode;
			}

			symbols.RemoveProcess(processId);
		}

		virtual void OnLoadDll(ULONG const processId, ULONG const threadId, LOAD_DLL_DEBUG_INFO const & info) const
		{
			static_cast<void>(processId);
			static_cast<void>(threadId);

			std::string const logicalName = FileSystem::PathOfFileHandle(info.hFile, VOLUME_NAME_NT);
			std::string const name = FileSystem::NormalisePath(logicalName, driveMap);

			Debug::Stream() << "GDB LoadDll: " << name << " " << info.lpBaseOfDll << std::endl;
		}

		virtual void OnUnloadDll(ULONG const processId, ULONG const threadId, UNLOAD_DLL_DEBUG_INFO const & info) const
		{
			static_cast<void>(processId);
			static_cast<void>(threadId);

			Debug::Stream() << "GDB UnloadDll: " << info.lpBaseOfDll << std::endl;
		}

		virtual void OnCreateThread(ULONG const processId, ULONG const threadId, CREATE_THREAD_DEBUG_INFO const & info)
		{
			static_cast<void>(processId);
			static_cast<void>(threadId);

			Debug::Stream() << "GDB CreateThread: " << info.hThread << std::endl;
		}

		virtual void OnExitThread(ULONG const processId, ULONG const threadId, EXIT_THREAD_DEBUG_INFO const & info)
		{
			static_cast<void>(processId);
			static_cast<void>(threadId);

			Debug::Stream() << "GDB ThreadExit code: " << info.dwExitCode << std::endl;
		}

		virtual ULONG OnException(ULONG const processId, ULONG const threadId, EXCEPTION_DEBUG_INFO const & info)
		{
			static_cast<void>(processId);
			static_cast<void>(threadId);
			static_cast<void>(info);
			return DBG_EXCEPTION_NOT_HANDLED;
		}

		virtual void OnDebugString(ULONG const processId, ULONG const threadId, OUTPUT_DEBUG_STRING_INFO const & info)
		{
			static_cast<void>(threadId);

			auto const address = Detail::ConvertAddress(info.lpDebugStringData);

			std::string message;
			if (info.fUnicode != 0)
			{
				auto const size = static_cast<size_t>(info.nDebugStringLength / 2) - 1;
				std::vector<wchar_t> buffer(size); // soh?
				symbols.GetProcess(processId).Process().ReadMemory<wchar_t>(address, &buffer[0], size);
				message = Cvt::W2A(std::wstring {&buffer[0], size});
			}
			else
			{
				size_t const size = info.nDebugStringLength - 1;
				std::vector<char> buffer(size); // soh?
				symbols.GetProcess(processId).Process().ReadMemory<char>(address, &buffer[0], size);
				message = std::string {&buffer[0], size};
			}

			message.erase(std::remove(message.begin(), message.end(), '\r'), message.end());
			GLib::Util::Splitter const splitter(message, "\n");
			for (auto it = splitter.begin(), end = splitter.end(); it != end;)
			{
				std::string const s = *it;
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