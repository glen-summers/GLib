#include "pch.h"

#include "Coverage.h"
#include "Function.h"

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
	std::string const coverageReport = CreateReport(processId); // todo just cache data to memory, and do report at exit
	std::filesystem::path path(GLib::Cvt::a2w(reportPath));
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

std::string Coverage::CreateReport(unsigned int processId)
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

	// merge templates here. then maybe change above loop to do it?
	std::set<Function> nameToFunction;
	for (const auto & p : indexToFunction)
	{
		const Function & function = p.second;
		const auto & it = nameToFunction.find(function);
		if (it == nameToFunction.end())
		{
			nameToFunction.insert(function);
		}
		else
		{
			it->Merge(p.second);
		}
	}

	size_t allLines{}, coveredLines{};
	for (const auto & x : nameToFunction)
	{
		allLines += x.AllLines();
		coveredLines += x.CoveredLines();
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

	// report generator AVs without these, todo calculate them
	p.PushAttribute("block_coverage", "0"); 
	p.PushAttribute("line_coverage", "0");
	p.PushAttribute("blocks_covered", "0");
	p.PushAttribute("blocks_not_covered", "0");

	p.PushAttribute("lines_covered", coveredLines);
	p.PushAttribute("lines_partially_covered", coveredLines); //?
	p.PushAttribute("lines_not_covered", allLines - coveredLines);

	p.OpenElement("functions");
	size_t functionId{};
	for (const auto & idFunctionPair : nameToFunction)
	{
		const Function & function = idFunctionPair;
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

		for (const auto & allFileLines : function.FileLines())
		{
			// <range source_id = "23" covered = "yes" start_line = "27" start_column = "0" end_line = "27" end_column = "0" / >
			const std::wstring & fileName = allFileLines.first;
			const auto & lines = allFileLines.second;

			for (const auto & line : lines)
			{
				unsigned lineNumber = line.first;
				bool covered = line.second;

				p.OpenElement("range");
				p.PushAttribute("source_id", files[fileName]);
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
