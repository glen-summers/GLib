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

		inline CONTEXT GetContext(EXCEPTION_POINTERS const & exceptionInfo)
		{
			return NativeTryCatch([&] { return *exceptionInfo.ContextRecord; });
		}

		struct VirtualBase
		{
			VirtualBase() = delete;
			VirtualBase(VirtualBase const & other) = delete;
			VirtualBase(VirtualBase && other) = delete;
			VirtualBase & operator=(VirtualBase const & other) = delete;
			VirtualBase & operator=(VirtualBase && other) = delete;
			virtual ~VirtualBase() = delete;
		};

		inline bool GetCPlusPlusExceptionName(std::span<ULONG_PTR const> const & ei, std::string & name)
		{
			return NativeTryCatch(
				[&]
				{
					VirtualBase const & q = *WindowsCast<VirtualBase *>(ei[1]);
					auto const & t = typeid(q);
					name = t.name();
					return true;
				});
		}

		inline bool GetCPlusPlusExceptionNameEx(std::span<ULONG_PTR const> const & ei, std::string & name)
		{
			return NativeTryCatch(
				[&]
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
					ULONG_PTR const instance = ei[instanceOffset64];
#elif
#error unexpected target
#endif

					std::span const throwInfo {WindowsCast<ULONG const *>(ei[throwInfoIndex]), catchableOffsetIndex + 1};

					ULONG const catchableOffset {throwInfo[catchableOffsetIndex]};
					std::span const catchables {WindowsCast<ULONG const *>(instance + catchableOffset), catchablesOffsetIndex + 1};

					ULONG const catchablesOffset {catchables[catchablesOffsetIndex]};
					std::span const catchablesTypes {WindowsCast<ULONG const *>(instance + catchablesOffset), typeInfoOffsetIndex + 1};
					ULONG const typeInfoOffset {catchablesTypes[typeInfoOffsetIndex]};

					name = WindowsCast<type_info const *>(instance + typeInfoOffset)->name();

					return true;
				});
		}

		inline void UnDecorate(std::string & symbolName)
		{
			constexpr ULONG undecoratedNameSize = 512;
			std::array<char, undecoratedNameSize> undecoratedName {};
			ULONG constexpr flags = UNDNAME_NAME_ONLY;

			if (!symbolName.empty() && *symbolName.begin() == '?' &&
					UnDecorateSymbolName(symbolName.c_str(), undecoratedName.data(), undecoratedNameSize, flags) != 0)
			{
				symbolName = undecoratedName.data();
			}
		}

		inline void Trace(std::ostream & stm, SymProcess const & process, Symbol const & symbol, uint64_t const address)
		{
			auto name = symbol.Name();
			UnDecorate(name);
			Formatter::Format(stm, "\t{0} + 0x{1:%X}\n", name, symbol.Displacement());
			if (auto const line = process.TryGetLineFromAddress(address))
			{
				Formatter::Format(stm, "\t{0}({1})", line->FileName(), line->LineNumber());
				if (line->Displacement() != 0)
				{
					stm << " + " << line->Displacement() << " byte(stm)";
				}
				stm << '\n';
			}
		}

		inline void InlineTrace(std::ostream & stm, SymProcess const & process, uint64_t const address, ULONG const inlineTrace)
		{
			ULONG inlineContext {};
			ULONG inlineFrameIndex {};

			if (SymQueryInlineTrace(process.Handle(), address, 0, address, address, &inlineContext, &inlineFrameIndex) == TRUE)
			{
				for (ULONG i = inlineContext; i < inlineContext + inlineTrace; ++i)
				{
					if (auto const symbol = process.TryGetSymbolFromInlineContext(address, i))
					{
						auto name = symbol->Name();
						UnDecorate(name);
						Formatter::Format(stm, "\tinline context {0} + 0x{1:%x}\n", name, symbol->Displacement());
						if (auto const line = process.TryGetLineFromInlineContext(address, i))
						{
							Formatter::Format(stm, "\t{0}({1})", line->FileName(), line->LineNumber());
							if (line->Displacement() != 0)
							{
								stm << " + " << line->Displacement() << " byte(stm)";
							}
							stm << '\n';
						}
					}
				}
			}
		}

		inline void WalkStack(std::ostream & stm, SymProcess const & process, ULONG const machineType, STACKFRAME64 * const frame,
													CONTEXT * const context, unsigned int const maxFrames)
		{
			for (unsigned int frames = 0; frames < maxFrames; ++frames)
			{
				if (StackWalk64(machineType, process.Handle(), GetCurrentThread(), frame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64,
												nullptr) == FALSE)
				{
					stm << "StackWalk64: " << GetLastError() << '\n';
					break;
				}

				uint64_t const address = frame->AddrPC.Offset;
				if (address == frame->AddrReturn.Offset)
				{
					break;
				}

				MEMORY_BASIC_INFORMATION mb {};
				if (VirtualQueryEx(process.Handle(), WindowsCast<PVOID>(address), &mb, sizeof mb) != 0)
				{
					auto * module = static_cast<HMODULE>(mb.AllocationBase);
					std::string const moduleName = FileSystem::PathOfModule(module);
					Formatter::Format(stm, "{0,-30} + 0x{1:%08X}\n", moduleName, address - WindowsCast<DWORD_PTR>(mb.AllocationBase));
				}

				ULONG const inlineTrace = SymAddrIncludeInlineTrace(process.Handle(), address);
				if (inlineTrace != 0)
				{
					InlineTrace(stm, process, address, inlineTrace);
				}
				else if (auto symbol = process.TryGetSymbolFromAddress(address))
				{
					Trace(stm, process, *symbol, address);
				}
			}
		}
	}

	inline void Print(std::ostream & stm, EXCEPTION_POINTERS const * exceptionInfo, unsigned int const maxFrames)
	{
		constexpr ULONG cPlusPlusExceptionNumber = 0xe06d7363;
		EXCEPTION_RECORD const * const er = exceptionInfo->ExceptionRecord;
		std::span const info {er->ExceptionInformation};

		Formatter::Format(stm, "Unhandled exception at {0} (code: {1:%08X})", er->ExceptionAddress, er->ExceptionCode);

		switch (er->ExceptionCode)
		{
			case STATUS_ACCESS_VIOLATION:
			{
				auto const msg = info[0] == 0 ? "reading"sv : "writing"sv;
				Formatter::Format(stm, " : Access violation {0} address {1}\n", msg, WindowsCast<PVOID>(info[1]));
				break;
			}

			case cPlusPlusExceptionNumber:
			{
				std::string name("<unknown>");
				if (!Detail::GetCPlusPlusExceptionName(info, name))
				{
					Detail::GetCPlusPlusExceptionNameEx(info, name);
				}
				Formatter::Format(stm, " : C++ exception of type: '{0}'\n", name);

				[[fallthrough]];
			}

			default:
			{
				if (er->NumberParameters != 0)
				{
					stm << "\tException parameters: ";
					for (ULONG i = 0; i < er->NumberParameters; ++i)
					{
						if (i != 0)
						{
							stm << ", ";
						}
						stm << WindowsCast<PVOID>(info[i]);
					}
					stm << std::endl;
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

		Detail::WalkStack(stm, SymProcess::CurrentProcess(), machineType, &frame, &context, maxFrames);
	}
}
