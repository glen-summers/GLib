#pragma once

#include "GLib/formatter.h"
#include "GLib/Win/Symbols.h"

#include "GLib/flogging.h"

#include <unordered_set>

namespace GLib::Win::Symbols
{
	namespace Detail
	{
		inline CONTEXT GetContext(struct _EXCEPTION_POINTERS * exceptionInfo)
		{
			CONTEXT context{};
			__try
			{
				context = *(exceptionInfo->ContextRecord);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{}
			return context;
		}

		inline bool GetCPlusPlusExceptionName(const ULONG_PTR ei[], std::string & name)
		{
			struct Log_vbase { virtual ~Log_vbase() = default; };

			bool ret = false;
			__try
			{
				const Log_vbase& q = *reinterpret_cast<Log_vbase*>(ei[1]);
				const type_info& t = typeid(q);
				name = t.name();
				ret = true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{}
			return ret;
		}

		inline bool GetCPlusPlusExceptionNameEx(const ULONG_PTR ei[], std::string & name)
		{
			bool ret = false;
			__try
			{
				// http://members.gamedev.net/sicrane/articles/exception.html
				// http://www.geoffchappell.com/studies/msvc/language/predefined/
				// https://blogs.msdn.microsoft.com/oldnewthing/20100730-00/?p=13273
				#if defined(_M_IX86)
				ULONG_PTR hinstance = 0;
				#elif defined(_M_X64)
				ULONG_PTR hinstance = ei[3];
				#endif
				const DWORD * throwInfo = reinterpret_cast<const DWORD *>(ei[2]);
				DWORD catchableOffset = throwInfo[3];
				const DWORD * catchables = reinterpret_cast<const DWORD *>(hinstance + catchableOffset);
				//DWORD nCatchables = catchables[0];
				DWORD catchablesOffset = catchables[1];
				const DWORD * catchablesTypes = reinterpret_cast<const DWORD *>(hinstance + catchablesOffset);
				DWORD typeInfoOffset = catchablesTypes[1];
				const type_info * catchablesType = reinterpret_cast<const type_info *>(hinstance + typeInfoOffset);
				name = catchablesType->name();
				ret = true;
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{}
			return ret;
		}

		inline void WalkStack(std::ostream & s, const SymProcess & sym, DWORD machineType, STACKFRAME64 * frame, CONTEXT * context, unsigned int maxFrames)
		{
			HANDLE h = sym.Handle().get();

			for (unsigned int frames = 0; frames < maxFrames; ++frames)
			{
				if (!::StackWalk64(machineType, h, ::GetCurrentThread(), frame, context, nullptr,
					SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
				{
					break;
				}

				DWORD64 address = frame->AddrPC.Offset;
				if (address == frame->AddrReturn.Offset)
				{
					break;
				}

				MEMORY_BASIC_INFORMATION mb = {};
				if (::VirtualQueryEx(h, reinterpret_cast<PVOID>(static_cast<DWORD_PTR>(address)), &mb, sizeof mb) != 0)
				{
					HMODULE module = static_cast<HMODULE>(mb.AllocationBase);
					std::string mod = FileSystem::PathOfModule(module);
					Formatter::Format(s, "{0,-30} + {1:%08X}\n", mod, static_cast<DWORD_PTR>(address) - reinterpret_cast<DWORD_PTR>(mb.AllocationBase));

					if (auto symbol = sym.TryGetSymbolFromAddress(address))
					{
						const char * symName = symbol->Name().c_str();
						char undecoratedName[512];

						if (symName[0] == '?')
						{
							DWORD flags = UNDNAME_NAME_ONLY;
							if (::UnDecorateSymbolName(symName, undecoratedName, sizeof(undecoratedName), flags) != 0)
							{
								symName = undecoratedName;
							}
						}

						Formatter::Format(s, "\t{0} + {1:%#x}\n", symName, symbol->Displacement());
						if (auto line = sym.TryGetLineFromAddress(address))
						{
							Formatter::Format(s, "\t{0}({1})", line->fileName, line->lineNumber);
							if (line->displacement != 0)
							{
								s << " + " << line->displacement << " byte(s)";
							}
							s << '\n';
						}
					}
				}
				else
				{
					s << "Stack walk ending - bad address: " << reinterpret_cast<PVOID>(static_cast<DWORD_PTR>(address)) << "\n";
					break;
				}
			}
		}
	}

	inline void Print(_EXCEPTION_POINTERS * exceptionInfo, std::ostream & s, unsigned int maxFrames)
	{
		constexpr DWORD CPLUSPLUS_EXCEPTION_NUMBER = 0xe06d7363;

		const _EXCEPTION_RECORD & er = *(exceptionInfo->ExceptionRecord);

		Formatter::Format(s, "Unhandled exception at {0} (code: {1:%08X})", er.ExceptionAddress, er.ExceptionCode);

		switch (er.ExceptionCode)
		{
			case STATUS_ACCESS_VIOLATION:
			{
				Formatter::Format(s, " : Access violation {0} address {1}\n", er.ExceptionInformation[0] == 0 ? "reading" : "writing", 
					reinterpret_cast<void*>(er.ExceptionInformation[1]));
				break;
			}

			case CPLUSPLUS_EXCEPTION_NUMBER:
			{
				std::string name("<unknown>");
				if (!Detail::GetCPlusPlusExceptionName(er.ExceptionInformation, name))
				{
					Detail::GetCPlusPlusExceptionNameEx(er.ExceptionInformation, name);
				}
				Formatter::Format(s, " : C++ exception of type: '{0}'\n", name);
			}

			// fall through
			default:
			{
				if (er.NumberParameters != 0)
				{
					s << "\tException parameters: ";
					for (DWORD i = 0; i < er.NumberParameters; ++i)
					{
						if (i != 0) s << ", ";
						Formatter::Format(s, "{0}", (void*)er.ExceptionInformation[i]);
					}
					s << std::endl;
				}
				break;
			}
		}

		STACKFRAME64 frame = {};
		CONTEXT context = Detail::GetContext(exceptionInfo);
		DWORD machineType;

#if defined(_M_IX86)
		machineType = IMAGE_FILE_MACHINE_I386;
		frame.AddrPC.Offset = context.Eip;
		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrFrame.Offset = context.Ebp;
		frame.AddrFrame.Mode = AddrModeFlat;
		frame.AddrStack.Offset = context.Esp;
		frame.AddrStack.Mode = AddrModeFlat;
#elif defined(_M_X64)
		machineType = IMAGE_FILE_MACHINE_AMD64;
		frame.AddrPC.Offset = context.Rip;
		frame.AddrPC.Mode = AddrModeFlat;
		frame.AddrFrame.Offset = context.Rbp;
		frame.AddrFrame.Mode = AddrModeFlat;
		frame.AddrStack.Offset = context.Rsp;
		frame.AddrStack.Mode = AddrModeFlat;
#else
		#error "Unsupported platform"
#endif

		Detail::WalkStack(s, SymProcess::CurrentProcess(), machineType, &frame, &context, maxFrames);
	}
}
