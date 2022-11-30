#pragma once

#include <GLib/Compat.h>
#include <GLib/Scope.h>
#include <GLib/Win/Local.h>
#include <GLib/Win/Process.h>

#define _NO_CVCONST_H // NOLINT (bugprone-reserved-identifier) required
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

#include <array>
#include <filesystem>
#include <functional>
#include <sstream>

static_assert(std::is_same_v<ULONG, DWORD>);
static_assert(std::is_same_v<ULONG64, DWORD64>);
static_assert(std::is_same_v<ULONG64, uint64_t>);

namespace GLib::Win::Symbols
{
	namespace Detail
	{
		struct Cleanup
		{
			void operator()(HANDLE const handle) const noexcept
			{
				Util::WarnAssertTrue(SymCleanup(handle), "SymCleanup");
			}
		};
		using SymbolHandle = std::unique_ptr<void, Cleanup>;

		inline Handle Duplicate(HANDLE const handle)
		{
			HANDLE duplicatedHandle = nullptr;
			Util::AssertTrue(DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &duplicatedHandle, 0, FALSE, DUPLICATE_SAME_ACCESS),
											 "DuplicateHandle");
			return Handle {duplicatedHandle};
		}

		inline auto ConvertBase(void * baseValue)
		{
			return Util::Detail::WindowsCast<uint64_t>(baseValue);
		}
	}

	class Symbol
	{
		ULONG index {};
		ULONG typeIndex {};
		enum SymTagEnum tag
		{
			SymTagNull
		};
		std::string name;
		uint64_t displacement {};

	public:
		Symbol(ULONG const index, ULONG const typeIndex, enum SymTagEnum const tag, std::string name, const uint64_t displacement)
			: index(index)
			, typeIndex(typeIndex)
			, tag(tag)
			, name(move(name))
			, displacement(displacement)
		{}

		Symbol(Symbol const &) = delete;
		Symbol(Symbol &&) = default;
		Symbol & operator=(Symbol const &) = delete;
		Symbol & operator=(Symbol &&) = delete;
		~Symbol() = default;

		[[nodiscard]] ULONG Index() const
		{
			return index;
		}

		void Index(ULONG const value)
		{
			index = value;
		}

		[[nodiscard]] ULONG TypeIndex() const
		{
			return typeIndex;
		}

		void TypeIndex(ULONG const value)
		{
			typeIndex = value;
		}

		[[nodiscard]] enum SymTagEnum Tag() const
		{
			return tag;
		}

		void Tag(enum SymTagEnum const value)
		{
			tag = value;
		}

		[[nodiscard]] std::string const & Name() const
		{
			return name;
		}

		void Name(std::string value)
		{
			name = move(value);
		}

		[[nodiscard]] uint64_t Displacement() const
		{
			return displacement;
		}
	};

	class Line
	{
		unsigned int lineNumber;
		std::string fileName;
		uint64_t address;
		unsigned int displacement;

	public:
		Line(unsigned int const lineNumber, std::string fileName, uint64_t const address, unsigned int const displacement)
			: lineNumber(lineNumber)
			, fileName(move(fileName))
			, address(address)
			, displacement(displacement)
		{}

		Line(Line const &) = delete;
		Line(Line &&) = default;
		Line & operator=(Line const &) = delete;
		Line & operator=(Line &&) = delete;
		~Line() = default;

		[[nodiscard]] unsigned int LineNumber() const
		{
			return lineNumber;
		}

		[[nodiscard]] std::string const & FileName() const
		{
			return fileName;
		}

		[[nodiscard]] uint64_t Address() const
		{
			return address;
		}

		[[nodiscard]] unsigned int Displacement() const
		{
			return displacement;
		}
	};

	class SymProcess
	{
		Process process;
		Detail::SymbolHandle symbolHandle;
		uint64_t baseOfImage {};

	public:
		SymProcess(Handle handle, uint64_t const baseOfImage)
			: process(std::move(handle))
			, symbolHandle(process.Handle().get())
			, baseOfImage(baseOfImage)
		{}

		static SymProcess CurrentProcess()
		{
			return GetProcess(GetCurrentProcess(), 0, true);
		}

		static SymProcess GetProcess(HANDLE const handle, uint64_t const baseOfImage, bool const invasive)
		{
			auto duplicate = Detail::Duplicate(handle);

			std::ostringstream searchPath;

			auto const processPath = Cvt::P2A(std::filesystem::path(FileSystem::PathOfProcessHandle(duplicate.get())).parent_path());
			searchPath << processPath << ";";

			// set sym opts, add exe to sym path, legacy code, still needed?
			if (auto const value = Compat::GetEnv("_NT_SYMBOL_PATH"))
			{
				searchPath << *value << ";";
			}

			if (auto const value = Compat::GetEnv("PATH"))
			{
				searchPath << *value << ";";
			}

			// when debugging processHandle and enumerateModules = true, get errorCode=0x8000000d : An illegal state change was requested
			Util::AssertTrue(SymInitializeW(duplicate.get(), Cvt::A2W(searchPath.str()).c_str(), invasive ? TRUE : FALSE), "SymInitializeW");
			return {std::move(duplicate), baseOfImage};
		}

		[[nodiscard]] HANDLE Handle() const
		{
			return process.Handle().get();
		}

		[[nodiscard]] Process const & Process() const
		{
			return process;
		}

		void ReadMemory(uint64_t const address, void * const buffer, size_t const size, bool const fromBase = true) const
		{
			process.ReadMemory(fromBase ? baseOfImage + address : address, buffer, size);
		}

		void WriteMemory(uint64_t const address, void const * const buffer, size_t const size, bool const fromBase = true) const
		{
			process.WriteMemory(fromBase ? baseOfImage + address : address, buffer, size);
		}

		template <typename T>
		[[nodiscard]] T Read(uint64_t const address, bool const absolute = false) const
		{
			T value;
			ReadMemory(address, &value, sizeof(T), absolute);
			return value;
		}

		template <typename T>
		void Write(uint64_t const address, T const & value, bool const absolute = false) const
		{
			WriteMemory(address, &value, sizeof(T), absolute);
		}

		[[nodiscard]] Symbol GetSymbolFromIndex(ULONG const index) const
		{
			std::array<SYMBOL_INFOW, 2 + MAX_SYM_NAME * sizeof(wchar_t) / sizeof(SYMBOL_INFO)> buffer {};
			auto * const symBuf = buffer.data();
			symBuf->SizeOfStruct = sizeof(SYMBOL_INFOW);
			symBuf->MaxNameLen = MAX_SYM_NAME;

			BOOL const result = SymFromIndexW(Handle(), baseOfImage, index, symBuf);
			Util::AssertTrue(result, "SymFromIndexW");

			return {symBuf->Index,
							symBuf->TypeIndex,
							static_cast<enum SymTagEnum>(symBuf->Tag),
							Cvt::W2A(std::wstring_view {static_cast<wchar_t const *>(symBuf->Name)}),
							{}};
		}

		[[nodiscard]] ULONG GetSymbolIdFromAddress(uint64_t const address) const
		{
			SYMBOL_INFOW buffer {};
			auto * const symBuf = &buffer;
			symBuf->SizeOfStruct = sizeof(SYMBOL_INFOW);
			uint64_t displacement = 0;
			BOOL const result = SymFromAddrW(Handle(), address, &displacement, symBuf);
			Util::AssertTrue(result, "SymFromAddrW");
			return symBuf->Index;
		}

		[[nodiscard]] std::optional<Symbol> TryGetSymbolFromAddress(uint64_t const address) const
		{
			std::array<SYMBOL_INFOW, 2 + MAX_SYM_NAME * sizeof(wchar_t) / sizeof(SYMBOL_INFO)> buffer {};
			auto * const symBuf = buffer.data();
			symBuf->SizeOfStruct = sizeof(SYMBOL_INFOW);
			symBuf->MaxNameLen = MAX_SYM_NAME;
			uint64_t displacement = 0;
			BOOL const result = SymFromAddrW(Handle(), address, &displacement, symBuf);
			if (Util::WarnAssertTrue(result, "SymFromAddrW"))
			{
				return Symbol {symBuf->Index, symBuf->TypeIndex, static_cast<enum SymTagEnum>(symBuf->Tag),
											 Cvt::W2A(std::wstring_view {static_cast<wchar_t const *>(symBuf->Name)}), displacement};
			}
			return {};
		}

		[[nodiscard]] std::optional<Symbol> TryGetSymbolFromInlineContext(uint64_t const address, ULONG const context) const
		{
			std::array<SYMBOL_INFOW, 2 + MAX_SYM_NAME * sizeof(wchar_t) / sizeof(SYMBOL_INFOW)> buffer {};
			auto * const symBuf = buffer.data();
			symBuf->SizeOfStruct = sizeof(SYMBOL_INFOW);
			symBuf->MaxNameLen = MAX_SYM_NAME;
			uint64_t displacement = 0;

			auto const result = SymFromInlineContextW(Handle(), address, context, &displacement, symBuf);
			if (Util::WarnAssertTrue(result, "SymFromInlineContext"))
			{
				return Symbol {symBuf->Index, symBuf->TypeIndex, static_cast<enum SymTagEnum>(symBuf->Tag),
											 Cvt::W2A(std::wstring_view {static_cast<wchar_t const *>(symBuf->Name)}), displacement};
			}
			return {};
		}

		[[nodiscard]] std::optional<Line> TryGetLineFromAddress(uint64_t const address) const
		{
			IMAGEHLP_LINEW64 tmpLine {sizeof(IMAGEHLP_LINEW64), {}, {}, {}, {}};
			ULONG displacement = 0;
			BOOL const result = SymGetLineFromAddrW64(Handle(), address, &displacement, &tmpLine);
			if (Util::WarnAssertTrue(result, "SymGetLineFromAddrW64"))
			{
				return Line {tmpLine.LineNumber, Cvt::W2A(tmpLine.FileName), tmpLine.Address, displacement};
			}
			return {};
		}

		[[nodiscard]] std::optional<Line> TryGetLineFromInlineContext(uint64_t const address, ULONG const inlineContext) const
		{
			IMAGEHLP_LINEW64 tmpLine {sizeof(IMAGEHLP_LINEW64), {}, {}, {}, {}};
			ULONG displacement = 0;
			BOOL const result = SymGetLineFromInlineContextW(Handle(), address, inlineContext, 0, &displacement, &tmpLine);
			if (Util::WarnAssertTrue(result, "SymGetLineFromInlineContext"))
			{
				return Line {tmpLine.LineNumber, Cvt::W2A(tmpLine.FileName), tmpLine.Address, displacement};
			}
			return {};
		}

		[[nodiscard]] std::optional<Symbol> TryGetClassParent(ULONG const symbolId) const
		{
			ULONG typeIndexOfClassParent {};

			// docs say TypeId param should be the TypeIndex member of returned SYMBOL_INFO
			// and the result from TI_GET_CLASSPARENTID is "The type index of the class parent."
			// so we then get TI_GET_SYMINDEX for indexOfClassParent
			// but seems to get indexOfClassParent having the same value of typeIndexOfClassParent
			if (SymGetTypeInfo(Handle(), baseOfImage, symbolId, TI_GET_CLASSPARENTID, &typeIndexOfClassParent) == FALSE)
			{
				return {};
			}

			ULONG indexOfClassParent {};
			if (SymGetTypeInfo(Handle(), baseOfImage, typeIndexOfClassParent, TI_GET_SYMINDEX, &indexOfClassParent) == FALSE)
			{
				return {};
			}

			Local<WCHAR> name;
			Util::AssertTrue(SymGetTypeInfo(Handle(), baseOfImage, indexOfClassParent, TI_GET_SYMNAME, GetAddress<WCHAR>(name).Raw()), "SymGetTypeInfo");

			return Symbol {indexOfClassParent, typeIndexOfClassParent, {}, Cvt::W2A(std::wstring_view {name.Get()}), {}};
		}

	private:
		[[nodiscard]] uint64_t Address(uint64_t const address, bool const fromBase) const
		{
			return fromBase ? baseOfImage + address : address;
		}
	};

	class Engine
	{
		std::unordered_map<ULONG, SymProcess> handles;

	public:
		Engine()
		{
			auto constexpr flags = SYMOPT_DEBUG | static_cast<ULONG>(SYMOPT_UNDNAME) | static_cast<ULONG>(SYMOPT_LOAD_LINES);
			SymSetOptions(SymGetOptions() | flags);
		}

		SymProcess & AddProcess(ULONG const processId, HANDLE const processHandle, const uint64_t baseOfImage, HANDLE const imageFile,
														std::string const & imageName)
		{
			SymProcess sp = SymProcess::GetProcess(processHandle, baseOfImage, false);

			uint64_t const loadBase = SymLoadModuleExW(sp.Handle(), imageFile, Cvt::A2W(imageName).c_str(), nullptr, baseOfImage, 0, nullptr, 0);
			Util::AssertTrue(0 != loadBase, "SymLoadModuleExW");

			return handles.emplace(processId, std::move(sp)).first->second;
		}

		[[nodiscard]] SymProcess const & GetProcess(ULONG const processId) const
		{
			auto const it = handles.find(processId);
			if (it == handles.end())
			{
				throw std::runtime_error("Process not found");
			}
			return it->second;
		}

		void RemoveProcess(ULONG const processId)
		{
			if (handles.erase(processId) == 0)
			{
				throw std::runtime_error("Process not found");
			}
		}

		void SourceFiles(std::function<void(PSOURCEFILEW)> f, HANDLE const process, void * base) const
		{
			static_cast<void>(this);
			Util::AssertTrue(SymEnumSourceFilesW(process, Detail::ConvertBase(base), nullptr, EnumSourceFiles, &f), "SymEnumSourceFilesW");
		}

		void Lines(std::function<void(PSRCCODEINFOW)> f, HANDLE const process, void * base) const
		{
			static_cast<void>(this);
			auto const result = SymEnumLinesW(process, Detail::ConvertBase(base), nullptr, nullptr, EnumLines, &f);
			Util::AssertTrue(result == TRUE || GetLastError() == ERROR_NOT_SUPPORTED, "SymEnumLinesW");
		}

		template <typename Inserter>
		void Processes(Inserter && inserter) const
		{
			static_cast<void>(this);
			std::function<void(HANDLE)> f = [&](HANDLE h) { *inserter++ = h; };
			Util::AssertTrue(SymEnumProcesses(EnumProcesses, &f), "SymEnumProcesses");
		}

	private:
		static BOOL CALLBACK EnumSourceFiles(SOURCEFILEW * const sourceFileInfo, void * const context)
		{
			(*static_cast<std::function<void(PSOURCEFILEW)> *>(context))(sourceFileInfo);
			return TRUE;
		}

		static BOOL CALLBACK EnumLines(SRCCODEINFOW * const lineInfo, void * const context)
		{
			(*static_cast<std::function<void(PSRCCODEINFOW)> *>(context))(lineInfo);
			return TRUE;
		}

		static BOOL CALLBACK EnumProcesses(HANDLE const hProcess, void * const context)
		{
			(*static_cast<std::function<void(HANDLE)> *>(context))(hProcess);
			return TRUE;
		}
	};
}