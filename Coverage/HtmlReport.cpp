#include "pch.h"

#include "Chunk.h"
#include "Directory.h"
#include "FunctionVisitor.h"
#include "Line.h"

#include "FileCoverageData.h"
#include "HtmlReport.h"
#include "RootDirs.h"

#include "resource.h"

#include <GLib/ConsecutiveFind.h>
#include <GLib/Cpp/HtmlGenerator.h>
#include <GLib/Formatter.h>
#include <GLib/Html/TemplateEngine.h>
#include <GLib/Win/Resources.h>
#include <GLib/Xml/Printer.h>

#include <fstream>
#include <ranges>
#include <set>

using GLib::Cvt::P2A;

std::string LoadHtml(unsigned int const idValue)
{
	return GLib::Win::LoadResourceString(nullptr, idValue, RT_HTML); // NOLINT bad macro
}

std::string GetDateTime(time_t const time)
{
	std::tm tmValue {};
	GLib::Compat::LocalTime(tmValue, time);
	std::ostringstream stm;
	stm << std::put_time(&tmValue, "%d %b %Y, %H:%M:%S (%z)");
	return stm.str();
}

HtmlReport::HtmlReport(std::string testName, std::filesystem::path const & htmlPath, CoverageData const & coverageData, bool const showWhiteSpace)
	: testName(std::move(testName))
	, time(GetDateTime(std::time(nullptr)))
	, htmlPath(htmlPath)
	, rootPaths(RootPaths(coverageData))
	, cssPath(Initialise(htmlPath))
	, rootTemplate(LoadHtml(IDR_ROOT_DIRECTORY))
	, dirTemplate(LoadHtml(IDR_DIRECTORY))
	, fileTemplate(LoadHtml(IDR_FILE))
	, functionsTemplate(LoadHtml(IDR_FUNCTIONS))
	, showWhiteSpace(showWhiteSpace)
{
	GLib::Flog::ScopeLog const scopeLog(log, GLib::Flog::Level::Info, "HtmlReport");

	Strings drives;
	for (auto const & rootPath : rootPaths)
	{
		if (!rootPath.is_absolute() && !rootPath.has_root_name())
		{
			throw std::runtime_error("Unexpected path: " + P2A(rootPath));
		}
		drives.insert(P2A(rootPath.root_name()));
	}
	bool const multipleDrives = drives.size() > 1;

	for (FileCoverageData const & data : coverageData | std::views::values)
	{
		auto [rootPath, subPath] = Reduce(data.Path(), rootPaths);

		if (multipleDrives)
		{
			auto const drive = P2A(rootPath.root_name()).substr(0, 1);
			subPath = std::filesystem::path {drive} / subPath;
		}
		GenerateSourceFile(subPath, data);
		index[subPath.parent_path()].push_back(data);
	}

	GenerateRootIndex();
	GenerateIndices();

	static_cast<void>(scopeLog);
}

std::filesystem::path HtmlReport::Initialise(std::filesystem::path const & path)
{
	std::string const styleSheet = LoadHtml(IDR_STYLESHEET);

	remove_all(path);
	create_directories(path);
	auto cssPath = path / "coverage.css";
	std::ofstream css(cssPath);
	if (!css)
	{
		throw std::runtime_error("Unable to create file");
	}
	css << styleSheet;
	return cssPath;
}

std::set<std::filesystem::path> HtmlReport::RootPaths(CoverageData const & data)
{
	std::set<std::filesystem::path> rootPaths;
	for (auto const & path : data | std::views::keys)
	{
		rootPaths.insert(path.parent_path());
	}
	RootDirectories(rootPaths);
	return rootPaths;
}

void HtmlReport::GenerateRootIndex() const
{
	unsigned int totalCoveredLines {};
	unsigned int totalCoverableLines {};
	unsigned int totalCoveredFunctions {};
	unsigned int totalCoverableFunctions {};

	std::vector<Directory> directories;

	for (auto const & [name, children] : index)
	{
		auto const & link = name / "index.html";

		unsigned int coveredLines {};
		unsigned int coverableLines {};
		unsigned int minChildPercent {HundredPercent};

		unsigned int coveredFunctions {};
		unsigned int coverableFunctions {};
		// minChildFunctionCover?

		for (FileCoverageData const & data : children)
		{
			coveredLines += data.CoveredLines();
			coverableLines += data.CoverableLines();

			auto const childPercent = Percentage(data.CoveredLines(), data.CoverableLines());
			minChildPercent = std::min(minChildPercent, childPercent);

			coveredFunctions += data.CoveredFunctions();
			coverableFunctions += data.CoverableFunctions();
		}

		directories.emplace_back(P2A(name), P2A(link), coveredLines, coverableLines, minChildPercent, coveredFunctions, coverableFunctions);

		totalCoveredLines += coveredLines;
		totalCoverableLines += coverableLines;
		totalCoveredFunctions += coveredFunctions;
		totalCoverableFunctions += coverableFunctions;
	}

	if (totalCoverableLines == 0)
	{
		throw std::runtime_error("Zero coverable lines");
	}

	auto const coveragePercent = Percentage(totalCoveredLines, totalCoverableLines);
	auto const coverageFunctionPercent = Percentage(totalCoveredFunctions, totalCoverableFunctions);

	GLib::Eval::Evaluator eval;
	eval.Set("title", testName);
	eval.Set("time", time);
	eval.Set("styleSheet", "./coverage.css");
	eval.Set("directories", directories);
	eval.Set("coveredLines", totalCoveredLines);
	eval.Set("coverableLines", totalCoverableLines);
	eval.Set("coveragePercent", coveragePercent);
	eval.Set("coverageStyle", GetCoverageLevel(coveragePercent));

	eval.Set("coveredFunctions", totalCoveredFunctions);
	eval.Set("coverableFunctions", totalCoverableFunctions);
	eval.Set("coverageFunctionsPercent", coverageFunctionPercent);
	eval.Set("coverageFunctionsStyle", GetCoverageLevel(coverageFunctionPercent));

	auto const rootIndex = htmlPath / "index.html";
	std::ofstream out(rootIndex);
	if (!out)
	{
		throw std::runtime_error("Unable to create output file : " + P2A(rootIndex));
	}

	GLib::Html::Generate(eval, rootTemplate, out);
}

// consolidate with GenerateRootIndex
void HtmlReport::GenerateIndices() const
{
	for (auto const & [subPath, children] : index)
	{
		std::vector<Directory> directories;

		unsigned int totalCoveredFunctions {};
		unsigned int totalCoverableFunctions {};
		unsigned int totalCoveredLines {};
		unsigned int totalCoverableLines {};

		for (FileCoverageData const & data : children)
		{
			totalCoveredLines += data.CoveredLines();
			totalCoverableLines += data.CoverableLines();
			totalCoveredFunctions += data.CoveredFunctions();
			totalCoverableFunctions += data.CoverableFunctions();
		}

		if (totalCoverableLines == 0)
		{
			throw std::runtime_error("Zero coverable lines");
		}

		auto const coveragePercent = Percentage(totalCoveredLines, totalCoverableLines);
		auto const coverageFunctionPercent = Percentage(totalCoveredFunctions, totalCoverableFunctions);

		for (FileCoverageData const & data : children)
		{
			std::string const text = P2A(data.Path().filename());
			directories.emplace_back(text, text + ".html", data.CoveredLines(), data.CoverableLines(), 0, data.CoveredFunctions(),
															 data.CoverableFunctions());
		}

		auto const path = htmlPath / subPath;
		auto const relativePath = relative(htmlPath, path);
		auto css = P2A(relativePath / "coverage.css");

		// in case no files generated, also todo, create error stub files
		create_directories(path);

		GLib::Eval::Evaluator eval;
		eval.Set("title", testName);
		eval.Set("time", time);
		eval.Set("index", P2A(relativePath / "index.html"));
		eval.Set("subPath", P2A(subPath));
		eval.Set("styleSheet", css);
		eval.Set("directories", directories);
		eval.Set("coveredLines", totalCoveredLines);
		eval.Set("coverableLines", totalCoverableLines);
		eval.Set("coveragePercent", coveragePercent);
		eval.Set("coverageStyle", GetCoverageLevel(coveragePercent));

		eval.Set("coveredFunctions", totalCoveredFunctions);
		eval.Set("coverableFunctions", totalCoverableFunctions);
		eval.Set("coverageFunctionsPercent", coverageFunctionPercent);
		eval.Set("coverageFunctionsStyle", GetCoverageLevel(coverageFunctionPercent));

		auto const & pathIndex = path / "index.html";
		std::ofstream out(pathIndex);
		if (!out)
		{
			throw std::runtime_error("Unable to create output file : " + P2A(pathIndex));
		}

		GLib::Html::Generate(eval, dirTemplate, out);
	}
}

void HtmlReport::GenerateSourceFile(std::filesystem::path const & subPath, FileCoverageData const & data) const
{
	auto const & targetPath = htmlPath / subPath;
	auto const & relativePath = relative(htmlPath, targetPath.parent_path());

	std::filesystem::path const & sourceFile = data.Path();
	std::ifstream const stm(sourceFile);
	if (!stm)
	{
		log.Warning("Unable to open input file : {0}", P2A(sourceFile));
		// generate error file
		return;
	}

	auto const & lineCoverage = data.LineCoverage();

	std::string source;
	{
		std::stringstream buffer;
		buffer << stm.rdbuf();

		try
		{
			std::stringstream tmp;
			Htmlify(buffer.str(), showWhiteSpace, tmp);
			source = tmp.str();
		}
		catch (std::exception const & e)
		{
			log.Warning("Failed to parse source file '{0}' : {1}", P2A(sourceFile), e.what());
			source = buffer.str();
		}
	}

	std::vector<Line> lines;

	for (auto const & sourceLine : GLib::Util::Splitter {source, "\n"})
	{
		auto const lineNumber = static_cast<unsigned int>(lines.size() + 1);
		LineCover cover {};
		auto const iter = lineCoverage.find(lineNumber);
		if (iter != lineCoverage.end())
		{
			cover = iter->second == 0 ? LineCover::NotCovered : LineCover::Covered;
		}
		lines.push_back({sourceLine, {}, {}, cover, {}});
	}

	auto const maxLineNumberWidth = static_cast<unsigned int>(floor(log10(lines.size()))) + 1;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::ostringstream paddedLineNumber;
		paddedLineNumber << std::setw(maxLineNumberWidth) << i + 1; // use a width format specifier in template?
		lines[i].PaddedNumber = paddedLineNumber.str();
		lines[i].Number = static_cast<unsigned int>(i + 1);
	}

	constexpr int effectiveHeaderLines = 10;
	constexpr int effectiveFooterLines = 3;
	auto const effectiveLines = lines.size() + effectiveHeaderLines + effectiveFooterLines;
	auto const ratio = HundredPercent / static_cast<float>(effectiveLines);

	auto const pred = [](Line const & line1, Line const & line2) { return line1.Cover != line2.Cover; };

	std::vector<Chunk> chunks;
	chunks.push_back({LineCover::None, effectiveHeaderLines * ratio});
	for (auto it = lines.begin(), end = lines.end(), next = end; it != end; it = next)
	{
		next = GLib::Util::ConsecutiveFind(it, end, pred);
		auto const size = static_cast<float>(std::distance(it, next));
		chunks.push_back({it->Cover, size * ratio});
	}
	chunks.push_back({LineCover::None, effectiveFooterLines * ratio});

	GLib::Eval::Evaluator eval;

	auto const parent = subPath.parent_path();
	auto const css = P2A(relativePath / "coverage.css");
	auto const coveragePercent = Percentage(data.CoveredLines(), lineCoverage.size());
	auto const coverageFunctionPercent = Percentage(data.CoveredFunctions(), data.CoverableFunctions());

	eval.Set("title", P2A(subPath));
	eval.Set("testName", testName);
	eval.Set("time", time);
	eval.Set("parent", P2A(parent));
	eval.Set("fileName", P2A(targetPath.filename()));
	eval.Set("styleSheet", css);
	eval.Set("coverageStyle", GetCoverageLevel(coveragePercent));

	eval.Set("coveredLines", data.CoveredLines());
	eval.Set("coverableLines", lineCoverage.size());
	eval.Set("coveragePercent", coveragePercent);

	eval.Set("coveredFunctions", data.CoveredFunctions());
	eval.Set("coverableFunctions", data.CoverableFunctions());
	eval.Set("coverageFunctionsPercent", coverageFunctionPercent);
	eval.Set("coverageFunctionsStyle", GetCoverageLevel(coverageFunctionPercent));

	eval.Set("index", P2A(relativePath / "index.html"));

	eval.SetCollection("lines", lines);
	eval.SetCollection("chunks", chunks);

	std::multiset<FunctionCoverage> coverage;
	for (auto const & func : data.Functions())
	{
		for (auto const & [file, l] : func.GetFileLines())
		{
			if (file == sourceFile)
			{
				unsigned int const oneBasedLine = l.begin()->first;

				// 0 can causes out of range for debug global delete, todo remove this and replace with jscript offset on navigate
				constexpr unsigned int functionOffset = 1;

				unsigned int zeroBasedLine {};
				if (oneBasedLine >= functionOffset)
				{
					zeroBasedLine = oneBasedLine - 1 - functionOffset;
				}

				lines[zeroBasedLine].HasLink = true;
				coverage.emplace(func.NameSpace(), func.ClassName(), func.FunctionName(), zeroBasedLine + 1, static_cast<unsigned int>(func.CoveredLines()),
												 static_cast<unsigned int>(func.AllLines()));
			}
		}
	}

	eval.SetCollection("functions", coverage);

	create_directories(targetPath.parent_path());
	{
		std::filesystem::path tmp = targetPath;
		tmp += L".html";
		std::ofstream out(tmp);
		if (!out)
		{
			throw std::runtime_error("Unable to create file");
		}
		GLib::Html::Generate(eval, fileTemplate, out);
	}

	{
		std::filesystem::path tmp = targetPath;
		tmp += L".functions.html";
		std::ofstream out(tmp);
		if (!out)
		{
			throw std::runtime_error("Unable to create file");
		}
		GLib::Html::Generate(eval, functionsTemplate, out);
	}
}