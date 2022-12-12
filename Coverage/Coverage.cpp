#include "pch.h"

#include "Coverage.h"

#include "FileCoverageData.h"
#include "Function.h"
#include "HtmlReport.h"
#include "SymbolNameUtils.h"

WideStrings Coverage::A2W(Strings const & strings)
{
	WideStrings wideStrings;
	std::ranges::transform(strings, std::inserter(wideStrings, wideStrings.begin()), GLib::Cvt::A2W);
	return wideStrings;
}

void Coverage::AddLine(std::wstring const & fileName, unsigned int const lineNumber, GLib::Win::Symbols::SymProcess const & symProcess,
											 uint64_t const address, Process & process)
{
	// filter out unknown source lines, looks like these cause the out of memory exceptions in ReportGenerator
	if (lineNumber == fooFoo || lineNumber == feeFee)
	{
		return;
	}

	if (fileName.find(L"predefined C++ types (compiler internal)") != std::string::npos) // move constant
	{
		return;
	}

	if (!wideFiles.contains(fileName))
	{
		bool include = includes.empty();
		bool exclude = false;

		for (auto const & value : includes)
		{
			bool const isMatch = _wcsnicmp(value.c_str(), fileName.c_str(), value.size()) == 0;
			include |= isMatch;
		}

		for (auto const & value : excludes)
		{
			bool const isMatch = _wcsnicmp(value.c_str(), fileName.c_str(), value.size()) == 0;
			exclude |= isMatch;
		}

		if (!include || exclude)
		{
			return;
		}

		if (!std::filesystem::path(fileName).is_absolute())
		{
			return;
		}

		// functional change to symbol api or compiler change? file names as of 28jan 2019 are all lower case
		wideFiles.insert(fileName);
	}

	auto iter = process.Addresses().find(address);
	if (iter == process.Addresses().end())
	{
		ULONG const symbolId = symProcess.GetSymbolIdFromAddress(address);
		auto const oldByte = symProcess.Read<unsigned char>(address);
		iter = process.AddAddress(address, Address {oldByte, symbolId});
	}

	Address::AddFileLine(fileName, lineNumber);
	symProcess.Write(address, debugBreakByte);
}

void Coverage::OnCreateProcess(ULONG const processId, ULONG const threadId, CREATE_PROCESS_DEBUG_INFO const & info)
{
	Debugger::OnCreateProcess(processId, threadId, info);
	GLib::Win::Symbols::SymProcess const & process = Symbols().GetProcess(processId);

	log.Info("+Process Pid:{0}, Path:{1}", processId, GLib::Win::FileSystem::PathOfProcessHandle(process.Process().Handle().get()));

	auto iter = processes.find(processId);
	if (iter == processes.end())
	{
		iter = processes.emplace(processId, processId).first;
	}

	CREATE_THREAD_DEBUG_INFO threadInfo {};
	threadInfo.hThread = info.hThread;
	OnCreateThread(processId, threadId, threadInfo);

	GLib::Flog::ScopeLog const scopeLog(log, GLib::Flog::Level::Info, "EnumLines");

	Symbols().Lines([&](SRCCODEINFOW const * const lineInfo)
									{ AddLine(&lineInfo->FileName[0], lineInfo->LineNumber, process, lineInfo->Address, iter->second); },
									process.Handle(), info.lpBaseOfImage);

	static_cast<void>(scopeLog);
}

void Coverage::CaptureData(ULONG const processId)
{
	GLib::Flog::ScopeLog const scopeLog(log, GLib::Flog::Level::Info, "CaptureData");

	GLib::Win::Symbols::SymProcess const & symProcess = Symbols().GetProcess(processId);
	auto const pit = processes.find(processId);
	if (pit == processes.end())
	{
		throw std::runtime_error("Process not found");
	}
	Process & process = pit->second;

	for (auto const & address : process.Addresses() | std::views::values)
	{
		auto symbolId = address.SymbolId();
		auto symbol = symProcess.GetSymbolFromIndex(symbolId);

		auto iter = process.IndexToFunction().find(symbolId);
		if (iter == process.IndexToFunction().end())
		{
			std::string parentName;
			std::optional<GLib::Win::Symbols::Symbol> const parent = symProcess.TryGetClassParent(symbolId);
			if (parent)
			{
				parentName = parent->Name();
			}

			auto const & [nameSpace, typeName, functionName] = CleanupFunctionNames(symbol.Name(), parentName);
			iter = process.AddFunction(symbol.Index(), nameSpace, typeName, functionName);
		}
		Function::Accumulate(address);
	}

	static_cast<void>(scopeLog);
}

void Coverage::OnExitProcess(ULONG const processId, ULONG const threadId, EXIT_PROCESS_DEBUG_INFO const & info)
{
	CaptureData(processId);
	Debugger::OnExitProcess(processId, threadId, info);
	log.Info("-Process Pid:{0}, Exited code: {1} ({1:%x})", processId, info.dwExitCode);
}

void Coverage::OnCreateThread(ULONG const processId, ULONG const threadId, CREATE_THREAD_DEBUG_INFO const & info)
{
	processes.at(processId).AddThread(threadId, info.hThread);
}

void Coverage::OnExitThread(ULONG const processId, ULONG const threadId, EXIT_THREAD_DEBUG_INFO const & info)
{
	static_cast<void>(info);
	processes.at(processId).RemoveThread(threadId);
}

ULONG Coverage::OnException(ULONG const processId, ULONG const threadId, EXCEPTION_DEBUG_INFO const & info)
{
	bool const isBreakpoint = info.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT;

	if (info.dwFirstChance != 0 && isBreakpoint)
	{
		uint64_t const addressValue = GLib::Win::Detail::ConvertAddress(info.ExceptionRecord.ExceptionAddress);
		auto const & process = processes.at(processId);

		auto const iter = process.Addresses().find(addressValue);
		if (iter != process.Addresses().end())
		{
			GLib::Win::Symbols::SymProcess const & symProcess = Symbols().GetProcess(processId);

			Address const & address = iter->second;
			symProcess.Write(addressValue, address.OldData());
			address.Visit();

			GLib::Win::HandleBase * const threadHandle = process.FindThread(threadId);

			CONTEXT ctx {};
			ctx.ContextFlags = CONTEXT_ALL; // NOLINT bad macro
			GLib::Win::Util::AssertTrue(GetThreadContext(threadHandle, &ctx), "GetThreadContext");
#ifdef _WIN64
			--ctx.Rip;
#elif _WIN32
			--ctx.Eip;
#endif
			GLib::Win::Util::AssertTrue(SetThreadContext(threadHandle, &ctx), "SetThreadContext");
		}
		return DBG_CONTINUE;
	}

	return Debugger::OnException(processId, threadId, info);
}

CoverageData Coverage::GetCoverageData() const
{
	GLib::Flog::ScopeLog const scopeLog(log, GLib::Flog::Level::Info, "GetCoverageData");

	CaseInsensitiveMap<wchar_t, Functions> fileNameToFunctionMap;

	for (auto const & process : processes | std::views::values)
	{
		for (auto const & function : process.IndexToFunction() | std::views::values)
		{
			for (auto const & fileName : Function::GetFileLines() | std::views::keys)
			{
				fileNameToFunctionMap[fileName].emplace(function);
			}
		}
	}

	CoverageData coverageData;

	for (auto const & [filePath, functions] : fileNameToFunctionMap)
	{
		auto fileIt = coverageData.find(filePath);
		if (fileIt == coverageData.end())
		{
			fileIt = coverageData.emplace(filePath, filePath).first;
		}

		FileCoverageData & fileCoverageData = fileIt->second;

		for (auto const & function : functions)
		{
			FileLines const & fileLines = Function::GetFileLines();

			auto justFileNameIt = fileLines.find(filePath);
			if (justFileNameIt == fileLines.end())
			{
				continue;
			}

			fileCoverageData.AddFunction(function);

			for (auto const & [line, covered] : justFileNameIt->second)
			{
				fileCoverageData.AddLine(line, covered);
			}
		}
	}

	static_cast<void>(scopeLog);
	return coverageData;
}

std::tuple<std::string, std::string, std::string> Coverage::CleanupFunctionNames(std::string const & name, std::string const & typeName) const
{
	std::string nameSpace;
	std::string functionName = name;
	std::string className = typeName;

	RemoveTemplateDefinitions(className);
	RemoveTemplateDefinitions(functionName);

	std::smatch match;
	if (!className.empty())
	{
		std::regex_search(className, match, nameSpaceRegex);
		if (!match.empty())
		{
			size_t const len = match[0].str().size();
			if (len >= 2)
			{
				nameSpace = className.substr(0, len - 2);

				if (className.compare(0, nameSpace.size(), nameSpace) == 0)
				{
					className.erase(0, len);
				}
				if (functionName.compare(0, nameSpace.size(), nameSpace) == 0)
				{
					functionName.erase(0, len);
				}
			}
			if (functionName.compare(0, className.size(), className) == 0)
			{
				functionName.erase(0, className.size() + 2);
			}
		}
	}
	else
	{
		std::regex_search(functionName, match, nameSpaceRegex);
		if (!match.empty())
		{
			size_t const len = match[0].str().size();
			if (len >= 2)
			{
				nameSpace = functionName.substr(0, len - 2);
				functionName.erase(0, len);
			}
		}
	}
	return {nameSpace, typeName, functionName};
}