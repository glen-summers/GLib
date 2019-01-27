#include "pch.h"

#include "Coverage.h"
#include "Function.h"
#include "RootDirs.h"

#include "HtmlReport.h"
#include "FileCoverageData.h"

#include <GLib/XmlPrinter.h>

#include <fstream>

WideStrings Coverage::a2w(const Strings& strings)
{
	WideStrings wideStrings;
	std::transform(strings.begin(), strings.end(), std::inserter(wideStrings, wideStrings.begin()), GLib::Cvt::a2w);
	return wideStrings;
}

void Coverage::AddLine(const std::wstring & fileName, unsigned lineNumber, const GLib::Win::Symbols::SymProcess & process, DWORD64 address)
{
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

		wideFiles.insert(fileName);
	}

	auto it = addresses.find(address);
	if (it == addresses.end())
	{
		const auto oldByte = process.Read<unsigned char>(address);
		it = addresses.insert({ address, oldByte }).first;
	}
	it->second.AddFileLine(fileName, lineNumber);

	process.Write(address, debugBreakByte);
}

void Coverage::OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO& info)
{
	Debugger::OnCreateProcess(processId, threadId, info);

	const GLib::Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);

	// todo, handle child processes, currently disabled via DEBUG_ONLY_THIS_PROCESS

	// timing check, 6s with a2w, 1s with no convert, unordered map 1.6s (needs tolower on string for hash)
	//using Clock = std::chrono::high_resolution_clock;
	//auto startValue = Clock::now();

	Symbols().Lines([&](PSRCCODEINFOW lineInfo) noexcept
	{
		// filter out unknown source lines, looks like these cause the out of memory exceptions in ReportGenerator
		if (lineInfo->LineNumber == 0xf00f00 || lineInfo->LineNumber == 0xfeefee)
		{
			return;
		}
		AddLine(lineInfo->FileName, lineInfo->LineNumber, process, lineInfo->Address);
	}, process.Handle().get(), info.lpBaseOfImage);

	// use flog...
	// auto now = Clock::now();
	// std::chrono::duration<double> elapsedSeconds = now - startValue;
	// auto elapsed = elapsedSeconds.count();
	// std::cout << "Symbol lines processed in " << elapsed << " s" << std::endl;

	CREATE_THREAD_DEBUG_INFO threadInfo {};
	threadInfo.hThread = info.hThread;
	OnCreateThread(processId, threadId, threadInfo);
}

void Coverage::OnExitProcess(DWORD processId, DWORD threadId, const EXIT_PROCESS_DEBUG_INFO& info)
{
	CreateReport(processId); // todo just cache data to memory, and do report at exit
	
	Debugger::OnExitProcess(processId, threadId, info);
}

void Coverage::OnCreateThread(DWORD processId, DWORD threadId, const CREATE_THREAD_DEBUG_INFO& info)
{
	UNREFERENCED_PARAMETER(processId);

	threads.insert({ threadId, info.hThread });
}

void Coverage::OnExitThread(DWORD processId, DWORD threadId, const EXIT_THREAD_DEBUG_INFO& info)
{
	UNREFERENCED_PARAMETER(processId);
	UNREFERENCED_PARAMETER(info);

	threads.erase(threadId);
}

DWORD Coverage::OnException(DWORD processId, DWORD threadId, const EXCEPTION_DEBUG_INFO& info)
{
	Debugger::OnException(processId, threadId, info);

	const bool isBreakpoint = info.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT;

	if (info.dwFirstChance!=0 && isBreakpoint)
	{
		const auto address = reinterpret_cast<uint64_t>(info.ExceptionRecord.ExceptionAddress);
		const auto it = addresses.find(address);
		if (it != addresses.end())
		{
			 const GLib::Win::Symbols::SymProcess & p = Symbols().GetProcess(processId);

			Address & a = it->second;
			p.Write(address, a.OldData());
			a.Visit();

			auto const tit = threads.find(threadId);
			if (tit == threads.end())
			{
				throw std::runtime_error("Thread not found");
			}

			CONTEXT ctx {};
			ctx.ContextFlags = CONTEXT_ALL;
			 GLib::Win::Util::AssertTrue(::GetThreadContext(tit->second, &ctx), "GetThreadContext");
#ifdef _WIN64
			--ctx.Rip;
#elif  _WIN32
			--ctx.Eip;
#endif
			 GLib::Win::Util::AssertTrue(::SetThreadContext(tit->second, &ctx), "SetThreadContext");
		}
		return DBG_CONTINUE;
	}

	return DBG_EXCEPTION_NOT_HANDLED;
}

void Coverage::CreateReport(unsigned int processId)
{
	const GLib::Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);

	std::map<ULONG, Function> indexToFunction;

	for (const auto & a : addresses)
	{
		GLib::Win::Symbols::Symbol symbol = process.GetSymbolFromAddress(a.first);
		const Address & address = a.second;

		auto it = indexToFunction.find(symbol.Index);
		if (it == indexToFunction.end())
		{
			GLib::Win::Symbols::Symbol parent;
			process.TryGetClassParent(symbol, parent);

			// ok need to merge multiple hits from templates, but not overloads?
			// store namespaceName+className+functionName+isTemplate
			 // if not a template then generate additional inserts? template specialisations?
			it = indexToFunction.insert({ symbol.Index, { std::move(symbol.name), std::move(parent.name) } }).first;
		}

		it->second.Accumulate(address);
	}

	// just do both for now
	CreateXmlReport(indexToFunction);
	CreateHtmlReport(indexToFunction, executable);
}

// move
void Coverage::CreateXmlReport(const std::map<ULONG, Function> & indexToFunction) const
{
	// merge templates here?
	// std::set<Function> nameToFunction;
	// for (const auto & p : indexToFunction)
	// {
	// 	const Function & function = p.second;
	// 	const auto & it = nameToFunction.find(function);
	// 	if (it == nameToFunction.end())
	// 	{
	// 		nameToFunction.insert(function);
	// 	}
	// 	else
	// 	{
	// 		it->Merge(p.second);
	// 	}
	// }

	size_t allLines{}, coveredLines{};
	for (const auto & x : indexToFunction)
	{
		allLines += x.second.AllLines();
		coveredLines += x.second.CoveredLines();
	}

	size_t fileId = 0;
	std::map<std::wstring, size_t> files;
	for (const auto & f : wideFiles)
	{
		files.insert({ f, fileId++ });
	}

	XmlPrinter p;

	p.PushDeclaration();
	p.OpenElement("results");
	p.OpenElement("modules");
	p.OpenElement("module");
	p.PushAttribute("name", std::filesystem::path(GLib::Cvt::a2w(executable)).filename().u8string());
	p.PushAttribute("path", executable);

	p.PushAttribute("id", "0"); // todo, hash of exe?

	// report generator AVs without these, todo calculate them?
	p.PushAttribute("block_coverage", "0");
	p.PushAttribute("line_coverage", "0");
	p.PushAttribute("blocks_covered", "0");
	p.PushAttribute("blocks_not_covered", "0");

	p.PushAttribute("lines_covered", coveredLines);
	p.PushAttribute("lines_partially_covered", coveredLines); //?
	p.PushAttribute("lines_not_covered", allLines - coveredLines);

	p.OpenElement("functions");
	size_t functionId{};
	for (const auto & idFunctionPair : indexToFunction)
	{
		const Function & function = idFunctionPair.second;
		p.OpenElement("function");
		// id="3048656" name="TestCollision" namespace="Sat" type_name="" block_coverage="0.00" line_coverage="0.00" blocks_covered="0" blocks_not_covered="30" lines_covered="0" lines_partially_covered="0" lines_not_covered="20">
		p.PushAttribute("id", functionId++);
		p.PushAttribute("name", function.FunctionName());
		p.PushAttribute("namespace", function.Namespace());
		p.PushAttribute("type_name", function.ClassName());

		// todo calculate these
		p.PushAttribute("block_coverage", "0");
		p.PushAttribute("line_coverage", "0");
		p.PushAttribute("blocks_covered", "0");
		p.PushAttribute("blocks_not_covered", "0");

		p.PushAttribute("lines_covered", function.CoveredLines());
		// todo p.PushAttribute("lines_partially_covered", "0");
		p.PushAttribute("lines_not_covered", function.AllLines() - function.CoveredLines());

		p.OpenElement("ranges");

		for (const auto & fileLines : function.FileLines())
		{
			// <range source_id = "23" covered = "yes" start_line = "27" start_column = "0" end_line = "27" end_column = "0" / >
			const std::wstring & fileName = fileLines.first;
			const auto & lines = fileLines.second;
			const size_t sourceId = files.find(fileName)->second; // check

			for (const auto & line : lines)
			{
				const unsigned lineNumber = line.first;
				const bool covered = line.second;

				p.OpenElement("range");
				
				p.PushAttribute("source_id", sourceId);
				p.PushAttribute("covered", covered ? "yes" : "no");
				p.PushAttribute("start_line", lineNumber);
				// todo? p.PushAttribute("start_column", "0");
				p.PushAttribute("end_line", lineNumber);
				// todo ?p.PushAttribute("end_column", "0");
				p.CloseElement(); // range
			}
		}
		p.CloseElement(); // ranges
		p.CloseElement(); // function
	}
	p.CloseElement(); // functions

	p.OpenElement("source_files");

	for (const auto & file : files)
	{
		p.OpenElement("source_file");
		p.PushAttribute("id", file.second);
		p.PushAttribute("path", GLib::Cvt::w2a(file.first));
		// todo? https://stackoverflow.com/questions/13256446/compute-md5-hash-value-by-c-winapi
		//p.PushAttribute("checksum_type", "MD5");
		//p.PushAttribute("checksum", "blah");
		p.CloseElement(); // source_file
	}
	p.Close();

	create_directories(reportPath);
	std::ofstream fs(reportPath / "Coverage.xml");
	fs << p.Xml();
}

void Coverage::CreateHtmlReport(const std::map<ULONG, Function> & indexToFunctionMap, const std::string & title) const
{
	HtmlReport report(title, reportPath / "HtmlReport", ConvertFunctionDataToFileData(indexToFunctionMap));
	(void)report; // class with no methods :(
}

std::map<std::filesystem::path, FileCoverageData> Coverage::ConvertFunctionDataToFileData(const std::map<ULONG, Function> & indexToFunctionMap)
{
	CaseInsensitiveMap<wchar_t, std::multimap<unsigned int, Function>> fileNameToFunctionMap; // just use map<path..>?

	for (const auto & it : indexToFunctionMap)
	{
		const Function & function = it.second;
		for (const auto & fileLineIt : function.FileLines())
		{
			const std::wstring & fileName = fileLineIt.first;
			const std::map<unsigned, bool> & lineCoverage = fileLineIt.second;
			unsigned int startLine = lineCoverage.begin()->first;
			fileNameToFunctionMap[fileName].insert({ startLine, function });
		}
	}

	std::map<std::filesystem::path, FileCoverageData> fileCoverageData;

	for (const auto & fd : fileNameToFunctionMap)
	{
		std::filesystem::path fileName = fd.first;
		const std::multimap<unsigned, Function> & startLineToFunctionMap = fd.second;

		auto & coverageData = fileCoverageData[fileName.wstring()];

		for (const auto & it : startLineToFunctionMap)
		{
			const Function & function = it.second;
			const FileLines & fileLines = function.FileLines();

			auto justFileNameIt = fileLines.find(fileName.wstring());
			if (justFileNameIt == fileLines.end())
			{
				continue;
			}

			for (const auto & lineHitPair : justFileNameIt->second)
			{
				bool covered = lineHitPair.second;
				size_t hitCount = coverageData.lineCoverage[lineHitPair.first] += covered ? 1 : 0;
				if (covered && hitCount == 1)
				{
					++coverageData.coveredLines;
				}
			}
		}
	}

	return fileCoverageData;
}

