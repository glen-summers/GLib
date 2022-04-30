#pragma once

#include <GLib/Win/FormatErrorMessage.h>
#include <GLib/Win/Symbols.h>

#include <GLib/Formatter.h>

#include <span>

namespace GLib::Win::Symbols
{
	// http://members.gamedev.net/sicrane/articles/exception.html
	// http://www.geoffchappell.com/studies/msvc/language/predefined/
	// https://blogs.msdn.microsoft.com/oldnewthing/20100730-00/?p=13273

	using namespace std::string_view_literals;
	using Util::Detail::WindowsCast;

	namespace Detail
	{
		template <typename Function>
		auto NativeTryCatch(Function function) -> decltype(function())
		{
			__try // NOLINT(clang-diagnostic-language-extension-token) required
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
			return NativeTryCatch([&]() { return *exceptionInfo.ContextRecord; });
		}

		struct VirtualBase
		{
			VirtualBase() = delete;
			VirtualBase(const VirtualBase & other) = delete;
			VirtualBase(VirtualBase && other) = delete;
			VirtualBase & operator=(const VirtualBase & other) = delete;
			VirtualBase & operator=(VirtualBase && other) = delete;
			virtual ~VirtualBase() = delete;
		};

		inline bool GetCPlusPlusExceptionName(const std::span<const ULONG_PTR> & ei, std::string & name)
		{
			return NativeTryCatch(
				[&]()
				{
					const VirtualBase & q = *WindowsCast<VirtualBase *>(ei[1]);
					const type_info & t = typeid(q);
					name = t.name();
					return true;
				});
		}

		inline bool GetCPlusPlusExceptionNameEx(const std::span<const ULONG_PTR> & ei, std::string & name)
		{
			return NativeTryCatch(
				[&]()
				{
					constexpr auto instanceOffset64 = 3;
					constexpr auto throwInfoIndex = 2;
					constexpr auto catchableOffsetIndex = 3;
					constexpr auto catchablesOffsetIndex = 1;
					constexpr auto typeInfoOffsetIndex = 1;

#if defined(_M_IX86)
					ULONG_PTR instance = 0;
					static_cast<void>(instanceOffset64);
#elif defined(_M_X64)
					ULONG_PTR instance = ei[instanceOffset64];
#elif
#error unexpected target
#endif

					const std::span throwInfo {WindowsCast<const ULONG *>(ei[throwInfoIndex]), catchableOffsetIndex + 1};

					const ULONG catchableOffset {throwInfo[catchableOffsetIndex]};
					const std::span catchables {WindowsCast<const ULONG *>(instance + catchableOffset), catchablesOffsetIndex + 1};

					const ULONG catchablesOffset {catchables[catchablesOffsetIndex]};
					const std::span catchablesTypes {WindowsCast<const ULONG *>(instance + catchablesOffset), typeInfoOffsetIndex + 1};
					const ULONG typeInfoOffset {catchablesTypes[typeInfoOffsetIndex]};

					name = WindowsCast<const type_info *>(instance + typeInfoOffset)->name();

					return true;
				});
		}

		inline void UnDecorate(std::string & symbolName)
		{
			constexpr ULONG undecoratedNameSize = 512;
			std::array<char, undecoratedNameSize> undecoratedName {};
			ULONG flags = UNDNAME_NAME_ONLY;

			if (!symbolName.empty() && *symbolName.begin() == '?' &&
					UnDecorateSymbolName(symbolName.c_str(), undecoratedName.data(), undecoratedNameSize, flags) != 0)
			{
				symbolName = undecoratedName.data();
			}
		}

		inline void Trace(std::ostream & s, const SymProcess & process, const Symbol & symbol, uint64_t address)
		{
			auto name = symbol.Name();
			UnDecorate(name);
			Formatter::Format(s, "\t{0} + 0x{1:%X}\n", name, symbol.Displacement());
			if (auto line = process.TryGetLineFromAddress(address))
			{
				Formatter::Format(s, "\t{0}({1})", line->FileName(), line->LineNumber());
				if (line->Displacement() != 0)
				{
					s << " + " << line->Displacement() << " byte(s)";
				}
				s << '\n';
			}
		}

		inline void InlineTrace(std::ostream & s, const SymProcess & process, uint64_t address, ULONG inlineTrace)
		{
			ULONG inlineContext {};
			ULONG inlineFrameIndex {};

			if (SymQueryInlineTrace(process.Handle(), address, 0, address, address, &inlineContext, &inlineFrameIndex) == TRUE)
			{
				for (ULONG i = inlineContext; i < inlineContext + inlineTrace; ++i)
				{
					if (auto symbol = process.TryGetSymbolFromInlineContext(address, i))
					{
						auto name = symbol->Name();
						UnDecorate(name);
						Formatter::Format(s, "\tinline context {0} + 0x{1:%x}\n", name, symbol->Displacement());
						if (auto line = process.TryGetLineFromInlineContext(address, i))
						{
							Formatter::Format(s, "\t{0}({1})", line->FileName(), line->LineNumber());
							if (line->Displacement() != 0)
							{
								s << " + " << line->Displacement() << " byte(s)";
							}
							s << '\n';
						}
					}
				}
			}
		}

		inline void WalkStack(std::ostream & s, const SymProcess & process, ULONG machineType, STACKFRAME64 * frame, CONTEXT * context,
													unsigned int maxFrames)
		{
			for (unsigned int frames = 0; frames < maxFrames; ++frames)
			{
				if (StackWalk64(machineType, process.Handle(), GetCurrentThread(), frame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64,
												nullptr) == FALSE)
				{
					s << "StackWalk64: " << GetLastError() << '\n';
					break;
				}

				uint64_t address = frame->AddrPC.Offset;
				if (address == frame->AddrReturn.Offset)
				{
					break;
				}

				MEMORY_BASIC_INFORMATION mb {};
				if (VirtualQueryEx(process.Handle(), WindowsCast<PVOID>(address), &mb, sizeof mb) != 0)
				{
					auto * module = static_cast<HMODULE>(mb.AllocationBase);
					std::string moduleName = FileSystem::PathOfModule(module);
					Formatter::Format(s, "{0,-30} + 0x{1:%08X}\n", moduleName, address - WindowsCast<DWORD_PTR>(mb.AllocationBase));
				}

				ULONG inlineTrace = SymAddrIncludeInlineTrace(process.Handle(), address);
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
		constexpr ULONG cPlusPlusExceptionNumber = 0xe06d7363;
		const EXCEPTION_RECORD * er = exceptionInfo->ExceptionRecord;
		const std::span info {er->ExceptionInformation};

		Formatter::Format(s, "Unhandled exception at {0} (code: {1:%08X})", er->ExceptionAddress, er->ExceptionCode);

		switch (er->ExceptionCode)
		{
			case STATUS_ACCESS_VIOLATION:
			{
				auto msg = info[0] == 0 ? "reading"sv : "writing"sv;
				Formatter::Format(s, " : Access violation {0} address {1}\n", msg, WindowsCast<PVOID>(info[1]));
				break;
			}

			case cPlusPlusExceptionNumber:
			{
				std::string name("<unknown>");
				if (!Detail::GetCPlusPlusExceptionName(info, name))
				{
					Detail::GetCPlusPlusExceptionNameEx(info, name);
				}
				Formatter::Format(s, " : C++ exception of type: '{0}'\n", name);

				[[fallthrough]];
			}

			default:
			{
				if (er->NumberParameters != 0)
				{
					s << "\tException parameters: ";
					for (ULONG i = 0; i < er->NumberParameters; ++i)
					{
						if (i != 0)
						{
							s << ", ";
						}
						s << WindowsCast<PVOID>(info[i]);
					}
					s << std::endl;
				}
				break;
			}
		}

		STACKFRAME64 frame = {};
		CONTEXT context = Detail::GetContext(*exceptionInfo);
		ULONG machineType {};

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
