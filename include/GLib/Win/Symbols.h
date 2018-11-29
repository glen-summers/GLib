#pragma once

#include "GLib/cvt.h"
#include "GLib/Win/Process.h"

#include <DbgHelp.h>
#pragma comment(lib , "DbgHelp.lib")

#include <sstream>
#include <functional>

namespace GLib
{
	namespace Win
	{
		namespace Symbols
		{
			namespace Detail
			{
				struct Cleanup
				{
					void operator()(HANDLE handle) const noexcept
					{
						Util::WarnAssertTrue(::SymCleanup(handle), "SymCleanup failed");
					}
				};
				typedef std::unique_ptr<void, Cleanup> SymbolHandle;
			}

			class Engine
			{
				Detail::SymbolHandle handle;

			public:
				Engine(HANDLE processHandle, bool enumerateModules = false)
					: handle(processHandle)
				{
					// set sym opts, add exe to sym path, legacy code, still needed?
					std::ostringstream searchPath;
					std::string value = Process::Detail::EnvironmentVariable("_NT_SYMBOL_PATH");
					if (!value.empty())
					{
						searchPath << value << ";";
					}
					value = Process::Detail::EnvironmentVariable("PATH");
					if (!value.empty())
					{
						searchPath << value << ";";
					}

					auto processPath = std::filesystem::path(FileSystem::PathOfProcessHandle(processHandle)).parent_path().u8string();
					searchPath << processPath << ";";

					// when debugging processHandle and enumerateModules = true
					// causes errorCode=0x8000000d : An illegal state change was requested
					// as does SymRefreshModuleList
					Util::AssertTrue(::SymInitializeW(handle.get(), Cvt::a2w(searchPath.str()).c_str(), enumerateModules),
						"SymInitialize failed");
				}

				void Refresh(HANDLE process)
				{
					Util::AssertTrue(::SymRefreshModuleList(process), "SymRefreshModuleList failed");
				}

				void Load(HANDLE process, HANDLE file, const std::string & name, const void * base)
				{
					Util::AssertTrue(0 != ::SymLoadModuleExW(process, file, Cvt::a2w(name).c_str(), nullptr,
						reinterpret_cast<DWORD64>(base), 0, nullptr, 0), "SymLoadModuleEx failed");
				}

				struct Line
				{
					DWORD64 ModuleBase;
					std::string ObjectFile;
					std::string FullFilename;
					DWORD LineNumber;
					DWORD64 Address;
				};

				template <typename inserter>
				void Lines(inserter && back_inserter, HANDLE process, void * base)
				{
					std::function<void(PSRCCODEINFOW)> f = [&](PSRCCODEINFOW lineInfo)
					{
						*back_inserter++ =
						{
							lineInfo->ModBase,
							Cvt::w2a(lineInfo->Obj),
							Cvt::w2a(lineInfo->FileName),
							lineInfo->LineNumber,
							lineInfo->Address
						};
					};
					Util::AssertTrue(::SymEnumLinesW(process, reinterpret_cast<ULONG64>(base), nullptr, nullptr, EnumLines, &f), "SymEnumLines failed");
				}
				
				template <typename inserter>
				void Processes(inserter && back_inserter)
				{
					std::function<void(HANDLE)> f = [&](HANDLE h)
					{
						*back_inserter++ = h;
					};
					Util::AssertTrue(::SymEnumProcesses(EnumProcesses, &f), "SymEnumLines failed");
				}

			private:
				static BOOL CALLBACK EnumLines(PSRCCODEINFOW lineInfo, PVOID context)
				{
					(*static_cast<std::function<void(PSRCCODEINFOW)>*>(context))(lineInfo);
					return TRUE;
				}

				static BOOL CALLBACK EnumProcesses(HANDLE hProcess, PVOID context)
				{
					(*static_cast<std::function<void(HANDLE)>*>(context))(hProcess);
					return TRUE;
				}
			};
		}
	}
}