#pragma once

#include "GLib/cvt.h"
#include "GLib/Win/Process.h"
#include "GLib/scope.h"

#define _NO_CVCONST_H
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

			struct Symbol
			{
				ULONG Index;
				ULONG TypeIndex;
				enum SymTagEnum Tag;
				std::string name;
			};

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

				Symbol GetSymbolFromAddress(uint64_t address) const
				{
					char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(wchar_t)];
					auto const symbol = reinterpret_cast<PSYMBOL_INFOW>(buffer);
					symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
					symbol->MaxNameLen = MAX_SYM_NAME;
					Util::AssertTrue(::SymFromAddrW(process.Handle().get(), address, nullptr, symbol), "SymFromAddr");
					return { symbol->Index, symbol->TypeIndex, static_cast<enum SymTagEnum>(symbol->Tag), Cvt::w2a(symbol->Name) };
				}

				bool TryGetClassParent(const Symbol & symbol, Symbol & result) const
				{
					DWORD typeIndexOfClassParent;

					// docs say TypeId param should be the TypeIndex member of returned SYMBOL_INFO
					// and the result from TI_GET_CLASSPARENTID is "The type index of the class parent."
					// so we then get TI_GET_SYMINDEX for indexOfClassParent
					// but seems to get indexOfClassParent having the same value of typeIndexOfClassParent
					if (!::SymGetTypeInfo(process.Handle().get(), baseOfImage, symbol.TypeIndex, TI_GET_CLASSPARENTID, &typeIndexOfClassParent))
					{
						return false;
					}

					DWORD indexOfClassParent;
					if (!::SymGetTypeInfo(process.Handle().get(), baseOfImage, typeIndexOfClassParent, TI_GET_SYMINDEX, &indexOfClassParent))
					{
						return false;
					}

					WCHAR * name = nullptr;
					SCOPE(freeName, [&]() // or wrap name in a Local holder
					{
							Util::Detail::Checker::WarnAssertTrue(::LocalFree(name) == nullptr, "LocalFree");
					});
					Util::AssertTrue(::SymGetTypeInfo(process.Handle().get(), baseOfImage, indexOfClassParent, TI_GET_SYMNAME, &name), "SymGetTypeInfo");

					result.Index = indexOfClassParent;
					result.TypeIndex = typeIndexOfClassParent;
					result.name = Cvt::w2a(name);
					return true;
				}

			private:
				uint64_t Address(uint64_t address, bool fromBase) const
				{
					return fromBase ? baseOfImage + address : address;
				}
			};

			class Engine
			{
				std::unordered_map<DWORD, SymProcess> handles;

			public:
#ifdef _DEBUG
				Engine()
				{
					::SymSetOptions(::SymGetOptions() | SYMOPT_DEBUG);
				}
#endif

				SymProcess & AddProcess(DWORD processId, HANDLE processHandle, uint64_t baseOfImage, HANDLE imageFile, const std::string & imageName)
				{
					Handle duplicate = Detail::Duplicate(processHandle);

					// set sym opts, add exe to sym path, legacy code, still needed?
					std::ostringstream searchPath;

					auto const processPath = std::filesystem::path(FileSystem::PathOfProcessHandle(duplicate.get())).parent_path().u8string();
					searchPath << processPath << ";";

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

					// when debugging processHandle and enumerateModules = true, get errorCode=0x8000000d : An illegal state change was requested
					Util::AssertTrue(::SymInitializeW(duplicate.get(), Cvt::a2w(searchPath.str()).c_str(), FALSE),
						"SymInitialize failed");

					DWORD64 const loadBase = ::SymLoadModuleExW(duplicate.get(), imageFile, Cvt::a2w(imageName).c_str(), nullptr,
						static_cast<DWORD64>(baseOfImage), 0, nullptr, 0);
						Util::AssertTrue(0 != loadBase, "SymLoadModuleEx failed");

					return handles.insert(std::make_pair(processId, SymProcess{ std::move(duplicate), baseOfImage })).first->second;
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
