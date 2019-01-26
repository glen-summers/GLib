#include "pch.h"

#include "Coverage.h"
#include "Function.h"
#include "RootDirs.h"
#include "HtmlPrinter.h"

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

	std::filesystem::path path(GLib::Cvt::a2w(reportPath)); // moveup
	create_directories(path);
	std::ofstream fs(path / "Coverage.xml");
	fs << p.Xml();
}

void Coverage::CreateHtmlReport(const std::map<ULONG, Function> & indexToFunctionMap, const std::string & title) const
{
	std::filesystem::path htmlPath(GLib::Cvt::a2w(reportPath)); // moveup
	htmlPath /= "HtmlReport";
	remove_all(htmlPath);
	create_directories(htmlPath);

	// just store in filesystem
	auto cssPath = htmlPath / "coverage.css";
	std::ofstream css(cssPath);
	css << R"(body {
  color: #000000;
  background-color: #FFFFFF;
}
td.title {
  text-align: center;
  font-size: 18pt;
}
span.line {
  color: blue;
}
tr.covered {
	background-color: #DCF4DC;
}
tr.notCovered {
	background-color: #F7DEDE;
}
div.box {
	width:100px; height:10px; border:1px solid #000
}
div.red {
	height:100%; background-color: #CE1620
}
div.amber {
	height:100%; background-color: #FF7518
}
div.green {
	height:100%; background-color: #03C03C
}
	)";
	css.close();

	CaseInsensitiveMap<wchar_t, std::multimap<unsigned int, Function>> fileNameToFunctionMap; // use map<path..>?

	// rootDirs assumes files are canonical and ignores case
	std::set<std::filesystem::path> rootPaths;

	for (const auto & it : indexToFunctionMap)
	{
		const Function & function = it.second;
		for (const auto & fileLineIt : function.FileLines())
		{
			const std::wstring & fileName = fileLineIt.first;
			const std::map<unsigned, bool> & lineCoverage = fileLineIt.second;

			rootPaths.insert(std::filesystem::path(fileName).parent_path());
			unsigned int startLine = lineCoverage.begin()->first;
			fileNameToFunctionMap[fileName].insert({ startLine, function });
		}
	}

	RootDirectories(rootPaths);

	std::map<std::filesystem::path, std::map<std::filesystem::path, unsigned int>> index;

	for (const auto & fd : fileNameToFunctionMap)
	{
		std::filesystem::path fileName = fd.first;
		const std::multimap<unsigned, Function> & startLineToFunctionMap = fd.second;

		size_t coveredLines{};
		std::map<unsigned int, size_t> lineCoverage;

		for (const auto & it : startLineToFunctionMap)
		{
			const Function & function = it.second;
			const FileLines & fileLines = function.FileLines();

			auto justFileNameIt = fileLines.find(fileName.wstring());
			if (justFileNameIt == fileLines.end())
			{
				continue;
			}

			for (const auto& lineHitPair : justFileNameIt->second)
			{
				bool covered = lineHitPair.second;
				size_t hitCount = lineCoverage[lineHitPair.first] += covered ? 1 : 0;
				if (covered && hitCount == 1)
				{
					++coveredLines;
				}
			}
		}

		std::filesystem::path subPath = Reduce(fileName, rootPaths);
		std::filesystem::path filePath = htmlPath / subPath;
		filePath.replace_filename(filePath.filename().wstring() + L".html");
		create_directories(filePath.parent_path());

		auto coveragePercent = static_cast<unsigned int>(100 * coveredLines / lineCoverage.size());

		GenerateHtmlFile(fileName, filePath, lineCoverage, cssPath, "Coverage - " + subPath.u8string(), coveragePercent);

		index[subPath.parent_path()].insert({ filePath.filename(), coveragePercent });
	}

	HtmlPrinter rootIndex("Coverage - " + title);
	rootIndex.OpenTable();

	for (const auto & pathChildrenPair : index)
	{
		const auto & relativePath = pathChildrenPair.first;
		const auto & children = pathChildrenPair.second;

		rootIndex.OpenElement("tr");
		rootIndex.OpenElement("td");
		auto relativePathIndex = relativePath / "index.html";
		rootIndex.Anchor(relativePathIndex, relativePath.u8string());
		rootIndex.CloseElement(false); // td
		rootIndex.CloseElement(); // tr

		HtmlPrinter childList("Coverage - " + title, cssPath);
		childList.OpenTable();

		for (const auto & child : children)
		{
			const auto & fileName = child.first;
			unsigned int coveragePercent = child.second;

			childList.OpenElement("tr");
			childList.OpenElement("td");
			childList.Anchor(fileName, fileName.u8string());
			childList.CloseElement(false); // td
			childList.OpenElement("td", false);
			AddHtmlCoverageBar(childList, coveragePercent);
			childList.CloseElement(false); // td
			childList.CloseElement(); // tr
		}
		childList.Close();
		auto pathIndex = htmlPath / relativePathIndex;
		std::ofstream out(pathIndex);
		if (out.fail())
		{
			throw std::runtime_error("Unable to create output file : " + pathIndex.u8string());
		}
		out << childList.Xml();
	}
	rootIndex.Close();

	auto indexFile = htmlPath / "index.html";
	std::ofstream out(indexFile);
	if (out.fail())
	{
		throw std::runtime_error("Unable to create output file : " + indexFile.u8string());
	}
	out << rootIndex.Xml();
}

void Coverage::GenerateHtmlFile(const std::filesystem::path & sourceFile, const std::filesystem::path & destFile, const std::map<unsigned int, size_t> & lines,
	const std::filesystem::path & css, const std::string & title, unsigned int coveragePercent) const
{
	// use template engine for boilerplate?
	std::ifstream in(sourceFile);
	if (in.fail())
	{
		throw std::runtime_error("Unable to open input file : " + sourceFile.u8string());
	}
	in.exceptions(std::ifstream::badbit);
	std::vector<std::string> fileLines;
	while (!in.eof())
	{
		std::string s;
		std::getline(in, s);
		fileLines.push_back(s);
	}

	HtmlPrinter printer(title, css);

	printer.OpenTable();
	printer.PushAttribute("width", "100%");

	printer.OpenElement("tr");
	printer.OpenElement("td");
	printer.PushAttribute("class", "title");
	printer.PushText("Coverage report");
	printer.CloseElement(); // td
	printer.CloseElement(); // tr

	printer.OpenElement("tr");
	printer.OpenElement("td");
	AddHtmlCoverageBar(printer, coveragePercent);
	printer.CloseElement(); // td
	printer.CloseElement(); // tr

	printer.CloseElement(); // table

	printer.OpenElement("pre");
	printer.OpenTable();

	for (size_t fileLine = 1; fileLine <= fileLines.size(); ++fileLine)
	{
		auto it = lines.find(static_cast<unsigned int>(fileLine));
		const char* style = nullptr;
		if (it != lines.end())
		{
			style = it->second != 0 ? "covered" : "notCovered";
		}
		std::string text = fileLines[fileLine - 1];
		size_t start = 0;
		while ((start = text.find('\t', start)) != std::string::npos)
		{
			text.replace(start, 1, "    ");
			start += 4;
		}

		printer.OpenElement("tr");
		if (style)
		{
			printer.PushAttribute("class", style);
		}
		printer.OpenElement("td", false);
		std::ostringstream l;
		l << std::setw(8) << fileLine << " : ";
		printer.Span(l.str(), "line");
		printer.PushText(text);
		//printer.PushText("\n"); // manual newline
		printer.CloseElement(false); // td
		printer.CloseElement(false); // tr
	}
	printer.Close(); 

	std::ofstream out(destFile);
	out << printer.Xml();
	if (out.fail())
	{
		throw std::runtime_error("Unable to create output file : " + destFile.u8string());
	}
}

void Coverage::AddHtmlCoverageBar(HtmlPrinter& printer, unsigned int percent)
{
	// move
	const int lowValue = 70;
	const int highValue = 90;
	const char* badStyle = "red";
	const char* warnStyle = "amber";
	const char* goodSttle = "green";

	std::ostringstream divWidth;
	divWidth << "width:" << percent << "px;";
	const char* divClass = percent < lowValue
		? badStyle
			: percent < highValue
			? warnStyle
				: goodSttle;

	printer.OpenElement("div");
	printer.PushAttribute("class", "box");
	printer.OpenElement("div");
	printer.PushAttribute("class", divClass);
	printer.PushAttribute("style", divWidth.str());
	printer.CloseElement(); // div
	printer.CloseElement(); // div
}
