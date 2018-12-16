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

				inline Handle Duplicate(HANDLE handle)
				{
					HANDLE duplicatedHandle;
					Util::AssertTrue(::DuplicateHandle(::GetCurrentProcess(), handle, ::GetCurrentProcess(),
						&duplicatedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS), "DuplicateHandle");
					return Handle { duplicatedHandle };
				}
			}

			class SymProcess
			{
				Process process;
				Detail::SymbolHandle symbolHandle;
				uint64_t baseOfImage;

			public:
				SymProcess(Handle && handle, uint64_t baseOfImage)
					: process(std::move(handle))
					, symbolHandle(process.Handle().get())
					, baseOfImage(baseOfImage)
				{}

				const Handle & Handle() const
				{
					return process.Handle();
				}

				void ReadMemory(uint64_t address, void * buffer, size_t size, bool fromBase = true) const
				{
					process.ReadMemory(fromBase ? baseOfImage + address : address, buffer, size);
				}

				void WriteMemory(uint64_t address, const void * buffer, size_t size, bool fromBase = true) const
				{
					process.WriteMemory(fromBase ? baseOfImage + address : address, buffer, size);
				}

				template <typename T> T Read(uint64_t address, bool absolute = false) const
				{
					T value;
					ReadMemory(address, &value, sizeof(T), absolute);
					return value;
				}

				template <typename T> void Write(uint64_t address, const T & value, bool absolute = false) const
				{
					WriteMemory(address, &value, sizeof(T), absolute);
				}

			private:
				uint64_t Address(uint64_t address, bool fromBase)
				{
					return fromBase ? baseOfImage + address : address;
				}
			};

			class Engine
			{
				std::unordered_map<DWORD, SymProcess> handles;

			public:
				SymProcess & AddProcess(DWORD processId, HANDLE processHandle, uint64_t baseOfImage, HANDLE imageFile, const std::string & imageName)
				{
					Handle duplicate = Detail::Duplicate(processHandle);

					// set sym opts, add exe to sym path, legacy code, still needed?
					std::ostringstream searchPath;
					std::string value = Win::Detail::EnvironmentVariable("_NT_SYMBOL_PATH");
					if (!value.empty())
					{
						searchPath << value << ";";
					}
					value = Win::Detail::EnvironmentVariable("PATH");
					if (!value.empty())
					{
						searchPath << value << ";";
					}

					auto const processPath = std::filesystem::path(FileSystem::PathOfProcessHandle(duplicate.get())).parent_path().u8string();
					searchPath << processPath << ";";

					// when debugging processHandle and enumerateModules = true, get errorCode=0x8000000d : An illegal state change was requested
					Util::AssertTrue(::SymInitializeW(duplicate.get(), Cvt::a2w(searchPath.str()).c_str(), FALSE),
						"SymInitialize failed");

					auto const ret = ::SymLoadModuleExW(duplicate.get(), imageFile, Cvt::a2w(imageName).c_str(), nullptr,
						static_cast<DWORD64>(baseOfImage), 0, nullptr, 0);
						Util::AssertTrue(0 != ret, "SymLoadModuleEx failed");

					auto it = handles.insert(std::make_pair(processId, SymProcess { std::move(duplicate), baseOfImage })).first;
					return it->second;
				}

				const SymProcess & GetProcess(DWORD processId) const
				{
					auto const it = handles.find(processId);
					if (it == handles.end())
					{
						throw std::runtime_error("Process not found");
					}
					return it->second;
				}

				void RemoveProcess(DWORD processId)
				{
					if (handles.erase(processId)!=1)
					{
						throw std::runtime_error("Process not found");
					}
				}

				void SourceFiles(std::function<void(PSOURCEFILEW)> f, HANDLE process, void * base) const
				{
					(void) this;
					Util::AssertTrue(::SymEnumSourceFilesW(process, reinterpret_cast<ULONG64>(base), nullptr, EnumSourceFiles, &f),
						"EnumSourceFiles failed");
				}

				void Lines(std::function<void(PSRCCODEINFOW)> f, HANDLE process, void * base) const
				{
					(void) this;
					Util::AssertTrue(::SymEnumLinesW(process, reinterpret_cast<ULONG64>(base), nullptr, nullptr, EnumLines, &f), "SymEnumLines failed");
				}
				
				template <typename inserter>
				void Processes(inserter && back_inserter) const
				{
					(void) this;
					std::function<void(HANDLE)> f = [&](HANDLE h) { *back_inserter++ = h; };
					Util::AssertTrue(::SymEnumProcesses(EnumProcesses, &f), "SymEnumLines failed");
				}

			private:
				static BOOL CALLBACK EnumSourceFiles(PSOURCEFILEW sourceFileInfo, PVOID context)
				{
					(*static_cast<std::function<void(PSOURCEFILEW)>*>(context))(sourceFileInfo);
					return TRUE;
				}

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
