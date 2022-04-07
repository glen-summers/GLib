#include "pch.h"

#include "Coverage.h"

#include "FileCoverageData.h"
#include "Function.h"
#include "HtmlReport.h"
#include "SymbolNameUtils.h"

WideStrings Coverage::A2W(const Strings & strings)
{
	WideStrings wideStrings;
	std::transform(strings.begin(), strings.end(), std::inserter(wideStrings, wideStrings.begin()), GLib::Cvt::A2W);
	return wideStrings;
}

void Coverage::AddLine(const std::wstring & fileName, unsigned int lineNumber, const GLib::Win::Symbols::SymProcess & symProcess, uint64_t address,
											 Process & process)
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

		for (const auto & value : includes)
		{
			const bool isMatch = _wcsnicmp(value.c_str(), fileName.c_str(), value.size()) == 0;
			include |= isMatch;
		}

		for (const auto & value : excludes)
		{
			const bool isMatch = _wcsnicmp(value.c_str(), fileName.c_str(), value.size()) == 0;
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

	auto it = process.Addresses().find(address);
	if (it == process.Addresses().end())
	{
		ULONG id = symProcess.GetSymbolIdFromAddress(address);
		const auto oldByte = symProcess.Read<unsigned char>(address);
		it = process.AddAddress(address, Address {oldByte, id});
	}
	it->second.AddFileLine(fileName, lineNumber);

	symProcess.Write(address, debugBreakByte);
}

void Coverage::OnCreateProcess(ULONG processId, ULONG threadId, const CREATE_PROCESS_DEBUG_INFO & info)
{
	Debugger::OnCreateProcess(processId, threadId, info);
	const GLib::Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);

	log.Info("+Process Pid:{0}, Path:{1}", processId, GLib::Win::FileSystem::PathOfProcessHandle(process.Process().Handle().get()));

	auto it = processes.find(processId);
	if (it == processes.end())
	{
		it = processes.emplace(processId, processId).first;
	}

	CREATE_THREAD_DEBUG_INFO threadInfo {};
	threadInfo.hThread = info.hThread;
	OnCreateThread(processId, threadId, threadInfo);

	GLib::Flog::ScopeLog scopeLog(log, GLib::Flog::Level::Info, "EnumLines");

	Symbols().Lines([&](PSRCCODEINFOW lineInfo) { AddLine(&lineInfo->FileName[0], lineInfo->LineNumber, process, lineInfo->Address, it->second); },
									process.Handle(), info.lpBaseOfImage);

	static_cast<void>(scopeLog);
}

void Coverage::CaptureData(ULONG processId)
{
	GLib::Flog::ScopeLog scopeLog(log, GLib::Flog::Level::Info, "CaptureData");

	const GLib::Win::Symbols::SymProcess & symProcess = Symbols().GetProcess(processId);
	auto pit = processes.find(processId);
	if (pit == processes.end())
	{
		throw std::runtime_error("Process not found");
	}
	Process & process = pit->second;

	for (const auto & [addressValue, address] : process.Addresses())
	{
		auto symbolId = address.SymbolId();
		auto symbol = symProcess.GetSymbolFromIndex(symbolId);

		auto it = process.IndexToFunction().find(symbolId);
		if (it == process.IndexToFunction().end())
		{
			std::string parentName;
			std::optional<GLib::Win::Symbols::Symbol> parent = symProcess.TryGetClassParent(symbolId);
			if (parent)
			{
				parentName = parent->Name();
			}

			const auto & [nameSpace, typeName, functionName] = CleanupFunctionNames(symbol.Name(), parentName);
			it = process.AddFunction(symbol.Index(), nameSpace, typeName, functionName);
		}
		it->second.Accumulate(address);
	}

	static_cast<void>(scopeLog);
}

void Coverage::OnExitProcess(ULONG processId, ULONG threadId, const EXIT_PROCESS_DEBUG_INFO & info)
{
	CaptureData(processId);
	Debugger::OnExitProcess(processId, threadId, info);
	log.Info("-Process Pid:{0}, Exited code: {1} ({1:%x})", processId, info.dwExitCode);
}

void Coverage::OnCreateThread(ULONG processId, ULONG threadId, const CREATE_THREAD_DEBUG_INFO & info)
{
	processes.at(processId).AddThread(threadId, info.hThread);
}

void Coverage::OnExitThread(ULONG processId, ULONG threadId, const EXIT_THREAD_DEBUG_INFO & info)
{
	static_cast<void>(info);
	processes.at(processId).RemoveThread(threadId);
}

ULONG Coverage::OnException(ULONG processId, ULONG threadId, const EXCEPTION_DEBUG_INFO & info)
{
	const bool isBreakpoint = info.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT;

	if (info.dwFirstChance != 0 && isBreakpoint)
	{
		const uint64_t address = GLib::Win::Detail::ConvertAddress(info.ExceptionRecord.ExceptionAddress);
		auto & process = processes.at(processId);

		auto it = process.Addresses().find(address);
		if (it != process.Addresses().end())
		{
			const GLib::Win::Symbols::SymProcess & p = Symbols().GetProcess(processId);

			const Address & a = it->second;
			p.Write(address, a.OldData());
			a.Visit();

			HANDLE threadHandle = process.FindThread(threadId);

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
	GLib::Flog::ScopeLog scopeLog(log, GLib::Flog::Level::Info, "GetCoverageData");

	CaseInsensitiveMap<wchar_t, Functions> fileNameToFunctionMap;

	for (const auto & [pid, process] : processes)
	{
		for (const auto & [id, function] : process.IndexToFunction())
		{
			for (const auto & [fileName, lineCoverage] : function.FileLines())
			{
				fileNameToFunctionMap[fileName].emplace(function);
			}
		}
	}

	CoverageData coverageData;

	for (const auto & [filePath, functions] : fileNameToFunctionMap)
	{
		auto fileIt = coverageData.find(filePath);
		if (fileIt == coverageData.end())
		{
			fileIt = coverageData.emplace(filePath, filePath).first;
		}

		FileCoverageData & fileCoverageData = fileIt->second;

		for (const auto & function : functions)
		{
			const FileLines & fileLines = function.FileLines();

			auto justFileNameIt = fileLines.find(filePath);
			if (justFileNameIt == fileLines.end())
			{
				continue;
			}

			fileCoverageData.AddFunction(function);

			for (const auto & [line, covered] : justFileNameIt->second)
			{
				fileCoverageData.AddLine(line, covered);
			}
		}
	}

	static_cast<void>(scopeLog);
	return coverageData;
}

std::tuple<std::string, std::string, std::string> Coverage::CleanupFunctionNames(const std::string & name, const std::string & typeName) const
{
	std::string nameSpace;
	std::string functionName = name;
	std::string className = typeName;

	RemoveTemplateDefinitions(className);
	RemoveTemplateDefinitions(functionName);

	std::smatch m;
	if (!className.empty())
	{
		std::regex_search(className, m, nameSpaceRegex);
		if (!m.empty())
		{
			size_t len = m[0].str().size();
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
		std::regex_search(functionName, m, nameSpaceRegex);
		if (!m.empty())
		{
			size_t len = m[0].str().size();
			if (len >= 2)
			{
				nameSpace = functionName.substr(0, len - 2);
				functionName.erase(0, len);
			}
		}
	}
	return {nameSpace, typeName, functionName};
}