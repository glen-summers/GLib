#include "pch.h"

#include "Coverage.h"

#include "FileCoverageData.h"
#include "Function.h"
#include "HtmlReport.h"

#include "GLib/Xml/Printer.h"

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

	auto it = process.addresses.find(address);
	if (it == process.addresses.end())
	{
		const auto oldByte = symProcess.Read<unsigned char>(address);
		it = process.addresses.emplace(address, oldByte).first;
	}
	it->second.AddFileLine(fileName, lineNumber);

	symProcess.Write(address, debugBreakByte);
}

void Coverage::OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO& info)
{
	Debugger::OnCreateProcess(processId, threadId, info);

	const GLib::Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);

	auto it = processes.find(processId);
	if (it == processes.end())
	{
		it = processes.emplace(processId, processId).first;
	}

	// timing check, 6s with a2w, 1s with no convert, unordered map 1.6s (needs tolower on string for hash)
	//using Clock = std::chrono::high_resolution_clock;
	//auto startValue = Clock::now();

	Symbols().Lines([&](PSRCCODEINFOW lineInfo)
	{
		AddLine(static_cast<const wchar_t *>(lineInfo->FileName), lineInfo->LineNumber, process, lineInfo->Address, it->second);
	}, process.Handle(), info.lpBaseOfImage);

	// use flog...
	// auto now = Clock::now();
	// std::chrono::duration<double> elapsedSeconds = now - startValue;
	// auto elapsed = elapsedSeconds.count();
	// std::cout << "Symbol lines processed in " << elapsed << " s" << std::endl;

	CREATE_THREAD_DEBUG_INFO threadInfo {};
	threadInfo.hThread = info.hThread;
	OnCreateThread(processId, threadId, threadInfo);
}

void Coverage::CaptureData(DWORD processId)
{
	const GLib::Win::Symbols::SymProcess & symProcess = Symbols().GetProcess(processId);
	auto pit = processes.find(processId);
	if (pit == processes.end())
	{
		throw std::runtime_error("Process not found");
	}

	Process & process = pit->second;

	for (const auto & a : process.addresses)
	{
		if (auto symbol = symProcess.TryGetSymbolFromAddress(a.first))
		{
			const Address & address = a.second;

			auto it = process.indexToFunction.find(symbol->Index());
			if (it == process.indexToFunction.end())
			{
				GLib::Win::Symbols::Symbol parent;
				symProcess.TryGetClassParent(*symbol, parent);

				std::string nameSpace;
				std::string typeName;
				std::string functionName;
				CleanupFunctionNames(symbol->Name(), parent.Name(), nameSpace, typeName, functionName);
				it = process.indexToFunction.emplace(symbol->Index(), Function{ nameSpace, typeName, functionName }).first;
			}

			it->second.Accumulate(address);
		}
	}
}

void Coverage::OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info)
{
	CaptureData(processId);
	Debugger::OnExitProcess(processId, threadId, info);
}

void Coverage::OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO& info)
{
	processes.at(processId).threads.emplace(threadId, info.hThread);
}

void Coverage::OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO& info)
{
	(void)info;
	processes.at(processId).threads.erase(threadId);
}

DWORD Coverage::OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO& info)
{
	const bool isBreakpoint = info.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT;

	if (info.dwFirstChance!=0 && isBreakpoint)
	{
		const auto address = GLib::Win::Detail::ConvertAddress(info.ExceptionRecord.ExceptionAddress);
		auto & process = processes.at(processId);
		const auto it = process.addresses.find(address);
		if (it != process.addresses.end())
		{
			const GLib::Win::Symbols::SymProcess & p = Symbols().GetProcess(processId);

			Address & a = it->second;
			p.Write(address, a.OldData());
			a.Visit();

			auto const tit = process.threads.find(threadId);
			if (tit == process.threads.end())
			{
				throw std::runtime_error("Thread not found");
			}

			CONTEXT ctx {};
			ctx.ContextFlags = CONTEXT_ALL; // NOLINT(hicpp-signed-bitwise) baad macro
			GLib::Win::Util::AssertTrue(::GetThreadContext(tit->second, &ctx), "GetThreadContext");
#ifdef _WIN64
			--ctx.Rip;
#elif _WIN32
			--ctx.Eip;
#endif
			 GLib::Win::Util::AssertTrue(::SetThreadContext(tit->second, &ctx), "SetThreadContext");
		}
		return DBG_CONTINUE;
	}

	return Debugger::OnException(processId, threadId, info);
}

CoverageData Coverage::GetCoverageData() const
{
	CaseInsensitiveMap<wchar_t, std::multimap<unsigned int, Function>> fileNameToFunctionMap; // just use map<path..>?

	for (const auto & [id, process] : processes)
	{
		for (const auto & [_, function] : process.indexToFunction)
		{
			for (const auto & [fileName, lineCoverage] : function.FileLines())
			{
				unsigned int startLine = lineCoverage.begin()->first;
				fileNameToFunctionMap[fileName].emplace(startLine, function);
			}
		}
	}

	CoverageData coverageData;

	for (const auto & fd : fileNameToFunctionMap)
	{
		const std::filesystem::path & filePath = fd.first;
		const std::multimap<unsigned, Function> & startLineToFunctionMap = fd.second;

		auto fileIt = coverageData.find(filePath);
		if (fileIt == coverageData.end())
		{
			fileIt = coverageData.emplace(filePath, filePath).first;
		}

		FileCoverageData & fileCoverageData = fileIt->second;

		for (const auto & it : startLineToFunctionMap)
		{
			const Function & function = it.second;
			const FileLines & fileLines = function.FileLines();

			auto justFileNameIt = fileLines.find(filePath.wstring());
			if (justFileNameIt == fileLines.end())
			{
				continue;
			}

			fileCoverageData.AddFunction(function);

			for (const auto & lineHitPair : justFileNameIt->second)
			{
				fileCoverageData.AddLine(lineHitPair.first, lineHitPair.second);
			}
		}
	}

	return move(coverageData);
}

void Coverage::Delaminate(std::string & name)
{
	for (size_t pos = name.find("<lambda"); pos != std::string::npos; pos = name.find("<lambda", pos))
	{
		name.erase(pos, 1);
		pos = name.find('>', pos);
		if (pos != std::string::npos)
		{
			name.erase(pos, 1);
		}
	}
}

void Coverage::CleanupFunctionNames(const std::string & name, const std::string & typeName,
	std::string & nameSpace, std::string & className, std::string & functionName) const
{
	className = typeName;
	functionName = name;
	Delaminate(className);
	Delaminate(functionName);

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