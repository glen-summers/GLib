#include "pch.h"

#include "Coverage.h"

#include "FileCoverageData.h"
#include "Function.h"
#include "HtmlReport.h"
#include "SymbolNameUtils.h"

#include <fstream>

WideStrings Coverage::a2w(const Strings& strings)
{
	WideStrings wideStrings;
	std::transform(strings.begin(), strings.end(), std::inserter(wideStrings, wideStrings.begin()), GLib::Cvt::a2w);
	return wideStrings;
}

void Coverage::AddLine(const std::wstring & fileName, unsigned lineNumber, const GLib::Win::Symbols::SymProcess & symProcess, DWORD64 address,
	Process & process)
{
	// filter out unknown source lines, looks like these cause the out of memory exceptions in ReportGenerator
	if (lineNumber == FooFoo || lineNumber == FeeFee)
	{
		return;
	}

	if (fileName.find(L"predefined C++ types (compiler internal)") != std::string::npos) // move constant
	{
		return;
	}

	if (wideFiles.find(fileName) == wideFiles.end())
	{
		bool include = includes.empty();
		bool exclude = false;

		for (const auto & value : includes)
		{
			bool const isMatch = ::_wcsnicmp(value.c_str(), fileName.c_str(), value.size()) == 0;
			include |= isMatch;
		}

		for (const auto & value : excludes)
		{
			bool const isMatch = ::_wcsnicmp(value.c_str(), fileName.c_str(), value.size()) == 0;
			exclude |= isMatch;
		}

		if (!include || exclude)
		{
			return;
		}

		// functional change to symbol api or compiler change? file names as of 28jan 2019 are all lower case
		wideFiles.insert(fileName);
	}

	auto it = process.Addresses().find(address);
	if (it == process.Addresses().end())
	{
		unsigned int id = symProcess.GetSymbolIdFromAddress(address);
		const auto oldByte = symProcess.Read<unsigned char>(address);
		it = process.AddAddress(address, Address{oldByte, id});
	}
	it->second.AddFileLine(fileName, lineNumber);

	symProcess.Write(address, debugBreakByte);
}

void Coverage::OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO& info)
{
	Debugger::OnCreateProcess(processId, threadId, info);
	const GLib::Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);

	log.Info("+Process Pid:{0}, Path:{1}", processId,
		GLib::Win::FileSystem::PathOfProcessHandle(process.Process().Handle().get()));

	auto it = processes.find(processId);
	if (it == processes.end())
	{
		it = processes.emplace(processId, processId).first;
	}

	CREATE_THREAD_DEBUG_INFO threadInfo {};
	threadInfo.hThread = info.hThread;
	OnCreateThread(processId, threadId, threadInfo);

	GLib::Flog::ScopeLog scopeLog(log, GLib::Flog::Level::Info, "EnumLines");
	(void)scopeLog;

	Symbols().Lines([&](PSRCCODEINFOW lineInfo)
	{
		AddLine(static_cast<const wchar_t *>(lineInfo->FileName), lineInfo->LineNumber, process, lineInfo->Address, it->second);
	}, process.Handle(), info.lpBaseOfImage);
}

void Coverage::CaptureData(DWORD processId)
{
	GLib::Flog::ScopeLog scopeLog(log, GLib::Flog::Level::Info, "CaptureData");
	(void)scopeLog;

	const GLib::Win::Symbols::SymProcess & symProcess = Symbols().GetProcess(processId);
	auto pit = processes.find(processId);
	if (pit == processes.end())
	{
		throw std::runtime_error("Process not found");
	}
	Process & process = pit->second;

	for (const auto & [addrValue, address] : process.Addresses())
	{
		auto symbolId = address.SymbolId();
		auto symbol = symProcess.GetSymbolFromIndex(symbolId);

		auto it = process.IndexToFunction().find(symbolId);
		if (it == process.IndexToFunction().end())
		{
			GLib::Win::Symbols::Symbol parent;
			symProcess.TryGetClassParent(symbolId, parent);

			std::string nameSpace;
			std::string typeName;
			std::string functionName;
			CleanupFunctionNames(symbol.Name(), parent.Name(), nameSpace, typeName, functionName);
			it = process.AddFunction(symbol.Index(), nameSpace, typeName, functionName);
		}
		it->second.Accumulate(address);
	}
}

void Coverage::OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info)
{
	CaptureData(processId);
	Debugger::OnExitProcess(processId, threadId, info);
	log.Info("-Process Pid:{0}, Exited code: {1} ({1:%x})", processId, info.dwExitCode);
}

void Coverage::OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO& info)
{
	processes.at(processId).AddThread(threadId, info.hThread);
}

void Coverage::OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO& info)
{
	(void)info;
	processes.at(processId).RemoveThread(threadId);
}

DWORD Coverage::OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO& info)
{
	const bool isBreakpoint = info.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT;

	if (info.dwFirstChance!=0 && isBreakpoint)
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
			ctx.ContextFlags = CONTEXT_ALL; // NOLINT(hicpp-signed-bitwise) baad macro
			GLib::Win::Util::AssertTrue(::GetThreadContext(threadHandle, &ctx), "GetThreadContext");
#ifdef _WIN64
			--ctx.Rip;
#elif _WIN32
			--ctx.Eip;
#endif
			 GLib::Win::Util::AssertTrue(::SetThreadContext(threadHandle, &ctx), "SetThreadContext");
		}
		return DBG_CONTINUE;
	}

	return Debugger::OnException(processId, threadId, info);
}

CoverageData Coverage::GetCoverageData() const
{
	GLib::Flog::ScopeLog scopeLog(log, GLib::Flog::Level::Info, "GetCoverageData");
	(void)scopeLog;

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

	return move(coverageData);
}

void Coverage::CleanupFunctionNames(const std::string & name, const std::string & typeName,
	std::string & nameSpace, std::string & className, std::string & functionName) const
{
	className = typeName;
	functionName = name;

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
}