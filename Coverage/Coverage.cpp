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

	auto it = process.addresses.find(address);
	if (it == process.addresses.end())
	{
		unsigned int id = symProcess.GetSymbolIdFromAddress(address);
		const auto oldByte = symProcess.Read<unsigned char>(address);
		it = process.addresses.emplace(address, Address{oldByte, id}).first;
		//log.Info("Pid: {1}, Address: {0:%x}", address, process.id);
	}
	//log.Info("Add Line:{0}, File:{1}", lineNumber, GLib::Cvt::w2a(fileName)); //tmp

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

	{
		GLib::Flog::ScopeLog linesScope(log, GLib::Flog::Level::Info, "EnumLines");
		Symbols().Lines([&](PSRCCODEINFOW lineInfo)
		{
			AddLine(static_cast<const wchar_t *>(lineInfo->FileName), lineInfo->LineNumber, process, lineInfo->Address, it->second);
		}, process.Handle(), info.lpBaseOfImage);
	}

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

	for (const auto & [addrValue, address] : process.addresses)
	{
		auto symbolId = address.SymbolId();
		auto symbol = symProcess.GetSymbolFromIndex(symbolId);

		auto it = process.indexToFunction.find(symbolId);
		if (it == process.indexToFunction.end())
		{
			GLib::Win::Symbols::Symbol parent;
			symProcess.TryGetClassParent(symbolId, parent);

			std::string nameSpace;
			std::string typeName;
			std::string functionName;
			CleanupFunctionNames(symbol.Name(), parent.Name(), nameSpace, typeName, functionName);

			auto ret = process.indexToFunction.emplace(symbol.Index(), Function{ symbol.Index(), nameSpace, typeName, functionName });
			if (!ret.second)
			{
				throw std::runtime_error(GLib::Formatter::Format("Duplicate function id:{0}, {1}:{2}:{3}",
					symbol.Index(), nameSpace, typeName, functionName));
			}
			it = ret.first;

			log.Info("CaptureData Pid: {4}, Id:{0} N:{1} T:{2} F:{3}", symbol.Index(), nameSpace, typeName, functionName, processId); // tmp
		}

		it->second.Accumulate(address);
	}
}

void Coverage::OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info)
{
	CaptureData(processId);
	Debugger::OnExitProcess(processId, threadId, info);
	log.Info("-Process Pid:{0}, Exited code: {1} ({1:%x})\n", processId, info.dwExitCode);
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
	CaseInsensitiveMap<wchar_t, Functions> fileNameToFunctionMap; // just use map id

	for (const auto & [pid, process] : processes)
	{
		for (const auto & [id, function] : process.indexToFunction)
		{
			for (const auto & [fileName, lineCoverage] : function.FileLines())
			{
				// 1. if function has overlaps log info
				// 1.1 currentlt need to iterate all existing functions for overlap, 1.2 use lookup
				// 2. try and merge

				//unsigned int startLine = lineCoverage.begin()->first;
				fileNameToFunctionMap[fileName].emplace(id, function); // are function ids unique accross processes?
			}
		}
	}

	CoverageData coverageData;

	for (const auto & [filePath, idToFunctionMap] : fileNameToFunctionMap)
	{
		auto fileIt = coverageData.find(filePath);
		if (fileIt == coverageData.end())
		{
			fileIt = coverageData.emplace(filePath, filePath).first;
		}

		FileCoverageData & fileCoverageData = fileIt->second;

		for (const auto & it : idToFunctionMap)
		{
			const Function & function = it.second;
			const FileLines & fileLines = function.FileLines();

			auto justFileNameIt = fileLines.find(filePath);
			if (justFileNameIt == fileLines.end())
			{
				continue;
			}

			std::ostringstream s;
			size_t count{};
			for (const auto & [f, l] : function.FileLines())
			{
				count++;
				s << GLib::Cvt::w2a(f) << " : ";
				for (const auto & [ll, bb] : l)
				{
					s << ", (" << ll << ":" << bb << ')';
				}
			}
			log.Info("GetCoverageData Id:{0} N:'{1}' T:'{2}' F:'{3}' '{4}'", function.Id(), function.NameSpace(), function.ClassName(), function.FunctionName(), s.str());
			if (count != 1)
			{
				log.Info("GetCoverageData gt {0}", count);
			}

			// try here: if function line range overlaps
			// need to keep list of ranges, and min\max line number in function

			fileCoverageData.AddFunction(function);

			for (const auto & [line, covered] : justFileNameIt->second)
			{
				fileCoverageData.AddLine(line, covered);
			}
		}
	}

	return move(coverageData);
}

void Coverage::RemoveTemplateDefinitions(std::string & name)
{
	constexpr char leftDoubleAngleQuote1 = '\xC2';
	constexpr char leftDoubleAngleQuote2 = '\xAB';
	if (name.find('<') == std::string::npos)
	{
		return;
	}

	auto pos = name.find("operator<<");
	if (pos!=std::string::npos)
	{
		name[pos+sizeof("operator<<")-3] = leftDoubleAngleQuote1;
		name[pos+sizeof("operator<<")-2] = leftDoubleAngleQuote2;
	}

	std::ostringstream s;
	size_t depth{};
	char lastChar{};

	for (auto c : name)
	{
		switch (c)
		{
			case '<':
			{
				if (depth++ == 0)
				{
					s.put(c);
					s.put('T');
				}
				break;
			}

			case '>':
			{
				// skip ->
				if (lastChar!= '-' && --depth == 0)
				{
					s.put(c);
				}
				break;
			}

			default:
			{
				if (depth == 0)
				{
					s.put(c);
				}
			}
		}
		lastChar = c;
	}

	if (depth != 0)
	{
		throw std::runtime_error("Unbalanced angle brackets : " + name);
	}
	name = s.str();
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