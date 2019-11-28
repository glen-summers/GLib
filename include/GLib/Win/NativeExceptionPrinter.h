#pragma once

#include "GLib/formatter.h"
#include "GLib/Span.h"

#include "GLib/Win/Symbols.h"

namespace GLib::Win::Symbols
{
	// http://members.gamedev.net/sicrane/articles/exception.html
	// http://www.geoffchappell.com/studies/msvc/language/predefined/
	// https://blogs.msdn.microsoft.com/oldnewthing/20100730-00/?p=13273

	namespace Detail
	{
		template <typename Function> auto NativeTryCatch(Function function) -> decltype(function())
		{
			__try
			{
				return function();
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				return {};
			}
		}

		inline CONTEXT GetContext(const EXCEPTION_POINTERS & exceptionInfo)
		{
			return NativeTryCatch([&]()
			{
				return *(exceptionInfo.ContextRecord);
			});
		}

		template <typename T1, typename T2> T1 Munge(T2 t2)
		{
			return reinterpret_cast<T1>(t2); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
		}

		struct Vbase
		{
			Vbase() = delete;
			Vbase(const Vbase & other) = delete;
			Vbase(Vbase && other) noexcept = delete;
			Vbase & operator=(const Vbase & other) = delete;
			Vbase & operator=(Vbase && other) noexcept = delete;
			virtual ~Vbase() = delete;
		};

		inline bool GetCPlusPlusExceptionName(const Span<ULONG_PTR> & ei, std::string & name)
		{
			return NativeTryCatch([&]()
			{
				const Vbase & q = *Munge<Vbase *>(ei[1]);
				const type_info & t = typeid(q);
				name = t.name();
				return true;
			});
		}

		inline bool GetCPlusPlusExceptionNameEx(const Span<ULONG_PTR> & ei, std::string & name)
		{
			return NativeTryCatch([&]()
			{
				constexpr auto instanceOffset64 = 3;
				constexpr auto throwInfoIndex = 2;
				constexpr auto catchableOffsetIndex = 3;
				constexpr auto catchablesOffsetIndex = 1;
				constexpr auto typeInfoOffsetIndex = 1;

#if defined(_M_IX86)
				ULONG_PTR hinstance = 0;
#elif defined(_M_X64)
				ULONG_PTR hinstance = ei[instanceOffset64];
#elif
#error unexpected target
#endif

				const auto throwInfo = GLib::MakeSpan<DWORD>(Munge<const DWORD *>(ei[throwInfoIndex]), catchableOffsetIndex+1);
				const DWORD catchableOffset = throwInfo[catchableOffsetIndex];
				const auto catchables = GLib::MakeSpan<DWORD>(Munge<const DWORD *>(hinstance + catchableOffset), catchablesOffsetIndex+1);
				const DWORD catchablesOffset = catchables[catchablesOffsetIndex];
				const auto catchablesTypes = GLib::MakeSpan<DWORD>(Munge<const DWORD *>(hinstance + catchablesOffset), typeInfoOffsetIndex+1);
				const DWORD typeInfoOffset = catchablesTypes[typeInfoOffsetIndex];
				name = Munge<const type_info *>(hinstance + typeInfoOffset)->name();
				return true;
			});
		}

		inline void UnDecorate(std::string & symbolName)
		{
			constexpr DWORD undecoratedNameSize = 512;
			std::array<char, undecoratedNameSize> undecoratedName{};
			DWORD flags = UNDNAME_NAME_ONLY;

			if (!symbolName.empty() && *symbolName.begin() == '?' && ::UnDecorateSymbolName(symbolName.c_str(), undecoratedName.data(), undecoratedNameSize, flags) != 0)
			{
				symbolName = undecoratedName.data();
			}
		}

		inline void Trace(std::ostream & s, const SymProcess & process, const Symbol & symbol, DWORD64 address)
		{
			auto name = symbol.Name();
			UnDecorate(name);
			Formatter::Format(s, "\t{0} + 0x{1:%X}\n", name, symbol.Displacement());
			if (auto line = process.TryGetLineFromAddress(address))
			{
				Formatter::Format(s, "\t{0}({1})", line->fileName, line->lineNumber);
				if (line->displacement != 0)
				{
					s << " + " << line->displacement << " byte(s)";
				}
				s << '\n';
			}
		}

		inline void InlineTrace(std::ostream & s, const SymProcess & process, DWORD64 address, DWORD inlineTrace)
		{
			DWORD inlineContext{};
			DWORD inlineFrameIndex{};

			if (::SymQueryInlineTrace(process.Handle(), address, 0, address, address, &inlineContext, &inlineFrameIndex) == TRUE)
			{
				for (DWORD i = inlineContext; i < inlineContext + inlineTrace; ++i)
				{
					if (auto symbol = process.TryGetSymbolFromInlineContext(address, i))
					{
						auto name = symbol->Name();
						UnDecorate(name);
						Formatter::Format(s, "\tinline context {0} + 0x{1:%x}\n", name, symbol->Displacement());
						if (auto line = process.TryGetLineFromInlineContext(address, i))
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
			}
		}

		inline void WalkStack(std::ostream & s, const SymProcess & process, DWORD machineType, STACKFRAME64 * frame, CONTEXT * context, unsigned int maxFrames)
		{
			for (unsigned int frames = 0; frames < maxFrames; ++frames)
			{
				if (::StackWalk64(machineType, process.Handle(), ::GetCurrentThread(), frame, context, nullptr,
					SymFunctionTableAccess64, SymGetModuleBase64, nullptr) == FALSE)
				{
					s << "StackWalk64: " <<  ::GetLastError() << '\n';
					break;
				}

				DWORD64 address = frame->AddrPC.Offset;
				if (address == frame->AddrReturn.Offset)
				{
					break;
				}

				MEMORY_BASIC_INFORMATION mb{};
				if (::VirtualQueryEx(process.Handle(), Munge<PVOID>(address), &mb, sizeof mb) != 0)
				{
					auto module = static_cast<HMODULE>(mb.AllocationBase);
					std::string moduleName = FileSystem::PathOfModule(module);
					Formatter::Format(s, "{0,-30} + 0x{1:%08X}\n", moduleName, static_cast<DWORD_PTR>(address) - Munge<DWORD_PTR>(mb.AllocationBase));
				}

				DWORD inlineTrace = ::SymAddrIncludeInlineTrace(process.Handle(), address);
				if (inlineTrace != 0)
				{
					InlineTrace(s, process, address, inlineTrace);
				}
				else if (auto symbol = process.TryGetSymbolFromAddress(address))
				{
					Trace(s, process, *symbol, address);
				}
			}
		}
	}

	inline void Print(std::ostream & s, const EXCEPTION_POINTERS * exceptionInfo, unsigned int maxFrames)
	{
		constexpr DWORD CPLUSPLUS_EXCEPTION_NUMBER = 0xe06d7363;

		const EXCEPTION_RECORD & er = *(exceptionInfo->ExceptionRecord);

		Formatter::Format(s, "Unhandled exception at {0} (code: {1:%08X})", er.ExceptionAddress, er.ExceptionCode);

		auto info = MakeSpan(static_cast<const ULONG_PTR *>(er.ExceptionInformation), EXCEPTION_MAXIMUM_PARAMETERS);

		switch (er.ExceptionCode)
		{
			case STATUS_ACCESS_VIOLATION:
			{
				auto msg = static_cast<const char *>(info[0] == 0 ? "reading" : "writing");
				Formatter::Format(s, " : Access violation {0} address {1}\n", msg, Detail::Munge<PVOID>(info[1]));
				break;
			}

			case CPLUSPLUS_EXCEPTION_NUMBER:
			{
				std::string name("<unknown>");
				if (!Detail::GetCPlusPlusExceptionName(info, name))
				{
					Detail::GetCPlusPlusExceptionNameEx(info, name);
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
						if (i != 0)
						{
							s << ", ";
						}
						s << Detail::Munge<PVOID>(info[i]);
					}
					s << std::endl;
				}
				break;
			}
		}

		STACKFRAME64 frame = {};
		CONTEXT context = Detail::GetContext(*exceptionInfo);
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
