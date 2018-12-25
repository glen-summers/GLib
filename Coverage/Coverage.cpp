#include "pch.h"

#include "Coverage.h"

#include <GLib/XmlPrinter.h>

#include <fstream>
//#include <iostream>

using GLib::Coverage;

void Coverage::OnCreateProcess(DWORD processId, DWORD threadId, const CREATE_PROCESS_DEBUG_INFO& info)
{
	Debugger::OnCreateProcess(processId, threadId, info);

	const Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);

	WideStrings wideFiles;

	// todo, handle child processes, currently disabled via DEBUG_ONLY_THIS_PROCESS

	// timing check, 6s with a2w, 1s with no convert, unordered map 1.6s (needs tolower on string for hash)
	//using Clock = std::chrono::high_resolution_clock;
	//auto startValue = Clock::now();

	Symbols().Lines([&](PSRCCODEINFOW lineInfo)
	{
		// filter out unknown source lines, looks like these cause the out of memory exceptions in ReportGenerator
		if (lineInfo->LineNumber == 0xf00f00 || lineInfo->LineNumber == 0xfeefee)
		{
			return;
		}

		if (wideFiles.find(lineInfo->FileName) == wideFiles.end())
		{
			bool include = includes.empty();
			bool exclude = false;

			for (const auto & value : includes)
			{
				bool const isMatch = ::_wcsnicmp(value.c_str(), lineInfo->FileName, value.size()) == 0;
				include |= isMatch;
			}

			for (const auto & value : excludes)
			{
				bool const isMatch = ::_wcsnicmp(value.c_str(), lineInfo->FileName, value.size()) == 0;
				exclude |= isMatch;
			}

			if (!include || exclude)
			{
				return;
			}

			wideFiles.insert(lineInfo->FileName);
		}

		const auto oldByte = process.Read<unsigned char>(lineInfo->Address);

		auto it = addresses.find(lineInfo->Address);
		if (it == addresses.end())
		{
			it = addresses.insert({ lineInfo->Address, oldByte}).first;
		}
		it->second.AddFileLine(lineInfo->FileName, lineInfo->LineNumber);

		process.Write(lineInfo->Address, debugBreakByte);
	}, process.Handle().get(), info.lpBaseOfImage);

	size_t fileId = 0;
	for (const auto & f : wideFiles)
	{
		files.insert({ f, fileId++ });
	}

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
	std::string const coverageReport = CreateReport(processId); // todo just cache data to memory, and do report at exit
	std::filesystem::path path(Cvt::a2w(reportPath));
	create_directory(path.parent_path());
	std::ofstream fs(path);
	fs << coverageReport;

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
	const bool isBreakpoint = info.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT;

	if (!isBreakpoint)
	{
		Debugger::OnException(processId, threadId, info); // for debug log
	}

	if (info.dwFirstChance!=0 && isBreakpoint)
	{
		const auto address = reinterpret_cast<uint64_t>(info.ExceptionRecord.ExceptionAddress);
		const auto it = addresses.find(address);
		if (it != addresses.end())
		{
			 const Win::Symbols::SymProcess & p = Symbols().GetProcess(processId);

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
			Win::Util::AssertTrue(::GetThreadContext(tit->second, &ctx), "GetThreadContext");
#ifdef _WIN64
			--ctx.Rip;
#elif  _WIN32
			--ctx.Eip;
#endif
			Win::Util::AssertTrue(::SetThreadContext(tit->second, &ctx), "SetThreadContext");
		}
		return DBG_CONTINUE;
	}

	Win::Debug::Stream() << "Exception: FirstChance: " << info.dwFirstChance << " " <<
		std::hex << info.ExceptionRecord.ExceptionCode << std::dec << std::endl;

	return DBG_EXCEPTION_NOT_HANDLED;
}

std::string Coverage::CreateReport(unsigned int processId)
{
	const Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);

	LinesCoverage allCoverage;

	size_t functionId = 0;
	std::map<ULONG, Function> indexToFunction;

	for (const auto & a : addresses)
	{
		Win::Symbols::Symbol symbol = process.GetSymbolFromAddress(a.first);
		const Address & address = a.second;
		bool visited = address.Visited();

		auto it = indexToFunction.find(symbol.Index);
		if (it == indexToFunction.end())
		{
			Win::Symbols::Symbol parent;
			std::smatch m;
			std::string functionName = symbol.name;
			std::string namespaceName;

			std::string className;
			if (process.TryGetClassParent(symbol, parent))
			{
				className = parent.name;
				std::regex_search(className, m, namespaceRegex);
				size_t len = m[0].str().size();
				if (len >= 2)
				{
					namespaceName = functionName.substr(0, len - 2);
					className.erase(0, len);
					functionName.erase(0, len);
				}
			}
			else
			{
				std::regex_search(functionName, m, namespaceRegex);
				size_t len = m[0].str().size();
				if (len >= 2)
				{
					namespaceName = functionName.substr(0, len - 2);
					functionName.erase(0, len);
				}
			}

			it = indexToFunction.insert({ symbol.Index, Function{functionId++, functionName, className, namespaceName, {}, {} } }).first;
		}

		FileLines & functionFileLines = it->second.fileLines;
		LinesCoverage & lines_coverage = it->second.coverage;

		for (const auto & fileLineIt : address.FileLines())
		{
			const auto & lines = fileLineIt.second;

			size_t lineCount = lines.size();
			if (visited)
			{
				allCoverage.linesCovered += lineCount;
				lines_coverage.linesCovered += lineCount;

				auto copy = lines;
				functionFileLines[fileLineIt.first].merge(copy);
			}
			else
			{
				allCoverage.linesNotCovered += lineCount;
				lines_coverage.linesNotCovered += lineCount;
			}
		}
	}

	XmlPrinter p;

	p.PushDeclaration();
	p.OpenElement("results");
	p.OpenElement("modules");
	p.OpenElement("module");
	p.PushAttribute("name", std::filesystem::path(Cvt::a2w(executable)).filename().u8string().c_str());
	p.PushAttribute("path", executable.c_str());

	p.PushAttribute("id", "0"); // todo, hash of exe?

	// report generator AVs without these, todo calculate them
	p.PushAttribute("block_coverage", "0"); 
	p.PushAttribute("line_coverage", "0");
	p.PushAttribute("blocks_covered", "0");
	p.PushAttribute("blocks_not_covered", "0");

	p.PushAttribute("lines_covered", allCoverage.linesCovered);
	p.PushAttribute("lines_partially_covered", allCoverage.linesCovered);
	p.PushAttribute("lines_not_covered", allCoverage.linesNotCovered);

	p.OpenElement("functions");
	for (const auto & idFunctionPair : indexToFunction) // sort
	{
		const Function & function = idFunctionPair.second;
		p.OpenElement("function");
		// id="3048656" name="TestCollision" namespace="Sat" type_name="" block_coverage="0.00" line_coverage="0.00" blocks_covered="0" blocks_not_covered="30" lines_covered="0" lines_partially_covered="0" lines_not_covered="20">
		p.PushAttribute("id", function.id);
		p.PushAttribute("name", function.name.c_str());
		p.PushAttribute("namespace", function.nameSpace.c_str());
		p.PushAttribute("type_name", function.typeName.c_str());

		// todo calculate these
		p.PushAttribute("block_coverage", "0");
		p.PushAttribute("line_coverage", "0");
		p.PushAttribute("blocks_covered", "0");
		p.PushAttribute("blocks_not_covered", "0");

		p.PushAttribute("lines_covered", function.coverage.linesCovered);
		// todo p.PushAttribute("lines_partially_covered", "0");
		p.PushAttribute("lines_not_covered", function.coverage.linesNotCovered);

		p.OpenElement("ranges");
		for (const auto & fileLines : function.fileLines)
		{
			// <range source_id = "23" covered = "yes" start_line = "27" start_column = "0" end_line = "27" end_column = "0" / >
			const std::wstring & fileName = fileLines.first;
			const auto & lines = fileLines.second;  // sort lines

			std::vector<unsigned> sortedLines{ lines.begin(), lines.end() };
			std::sort(sortedLines.begin(), sortedLines.end());

			for (unsigned line : sortedLines)
			{
				p.OpenElement("range");
				p.PushAttribute("source_id", files[fileName]);
				p.PushAttribute("covered", "yes");
				p.PushAttribute("start_line", line);
				// todo? p.PushAttribute("start_column", "0");
				p.PushAttribute("end_line", line);
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
		p.PushAttribute("path", Cvt::w2a(file.first).c_str());
		// todo? https://stackoverflow.com/questions/13256446/compute-md5-hash-value-by-c-winapi
		//p.PushAttribute("checksum_type", "MD5");
		//p.PushAttribute("checksum", "blah");
		p.CloseElement(); // source_file
	}
	p.CloseElement(); // source_files
	p.CloseElement(); // module
	p.CloseElement(); // modules
	p.CloseElement(); // results

	/* Try Cobertura version?
	p.OpenElement("coverage");
	p.PushAttribute("version", "1.9");
	p.PushAttribute("timestamp", std::time(nullptr));
	p.OpenElement("sources");
	p.OpenElement("source");
	p.PushText("srcFile");
	p.CloseElement(); // source
	p.CloseElement(); // sources

	p.OpenElement("packages");
	p.OpenElement("package");
	p.OpenElement("classes");
	p.OpenElement("class");
	p.OpenElement("methods");
	p.OpenElement("method");
	p.OpenElement("lines");
	p.OpenElement("line");
	p.CloseElement(); // line
	p.CloseElement(); // lines
	p.CloseElement(); // method
	p.CloseElement(); // methods
	p.CloseElement(); // class
	p.CloseElement(); // classes
	p.CloseElement(); // package
	p.CloseElement(); // packages
	p.CloseElement(); // coverage
	*/

	return p.Xml();
}
