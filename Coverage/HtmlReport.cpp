#include "pch.h"

#include "CppHtmlGenerator.h"
#include "Directory.h"
#include "FileCoverageData.h"
#include "HtmlReport.h"
#include "RootDirs.h"

#include "resource.h"

#include "GLib/Eval/Evaluator.h"
#include "GLib/Html/TemplateEngine.h"
#include "GLib/Win/Resources.h"
#include "GLib/Xml/Printer.h"
#include "GLib/formatter.h"

#include <fstream>
#include <set>

struct Line
{
	std::string text;
	std::string number;
	const char * style;
};

template <>
struct GLib::Eval::Visitor<Line>
{
	static void Visit(const Line & line, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "style")
		{
			return f(Value(line.style));
		}

		if (propertyName == "number")
		{
			return f(Value(line.number));
		}

		if (propertyName == "text")
		{
			return f(Value(line.text));
		}

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};

std::string LoadHtml(unsigned int id)
{
	return GLib::Win::LoadResourceString(nullptr, id, RT_HTML); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast) baad macro
}

std::string GetDateTime(time_t t)
{
	std::tm tm {};
	GLib::Compat::LocalTime(tm, t);
	std::ostringstream os;
	os << std::put_time(&tm, "%d %b %Y, %H:%M:%S (%z)");
	return os.str();
}

HtmlReport::HtmlReport(const std::string & testName, const std::filesystem::path & htmlPath,
	const std::map<std::filesystem::path, FileCoverageData> & fileCoverageData)
	: testName(testName)
	, time(GetDateTime(std::time(nullptr)))
	, htmlPath(htmlPath)
	, rootPaths(RootPaths(fileCoverageData))
	, cssPath(Initialise(htmlPath))
	, dirTemplate(LoadHtml(IDR_DIRECTORY))
	, fileTemplate(LoadHtml(IDR_FILE))
{
	for (const auto & fileDataPair : fileCoverageData)
	{
		const FileCoverageData & data = fileDataPair.second;

		auto subPath = Reduce(data.Path(), rootPaths);
		GenerateSourceFile(subPath, data);

		index[subPath.parent_path()].push_back(data);
	}
	GenerateRootIndex();
	GenerateIndices();
}

std::filesystem::path HtmlReport::Initialise(const std::filesystem::path & path)
{
	std::string s = LoadHtml(IDR_STYLESHEET);

	remove_all(path);
	create_directories(path);
	auto cssPath = path / "coverage.css";
	std::ofstream css(cssPath);
	if(!css)
	{
		throw std::runtime_error("Unable to create file");
	}
	css << s;
	return std::move(cssPath);
}

std::set<std::filesystem::path> HtmlReport::RootPaths(const std::map<std::filesystem::path, FileCoverageData>& data)
{
	std::set<std::filesystem::path> rootPaths;
	for (const auto & fileNameDataPair : data)
	{
		rootPaths.insert(std::filesystem::path(fileNameDataPair.first).parent_path());
	}
	RootDirectories(rootPaths);
	return rootPaths;
}

void HtmlReport::GenerateRootIndex() const
{
	unsigned int totalCoveredLines{};
	unsigned int totalCoverableLines{};
	std::vector<Directory> directories;

	for (const auto & pathChildrenPair : index)
	{
		const auto& name = pathChildrenPair.first;
		const auto& link = name / "index.html";
		const auto& children = pathChildrenPair.second;

		unsigned int coveredLines{};
		unsigned int coverableLines{};
		for (const FileCoverageData & data : children)
		{
			coveredLines += data.CoveredLines();
			coverableLines += data.CoverableLines();
		}
		directories.emplace_back(name.u8string(), link.generic_u8string(), coveredLines, coverableLines);
		totalCoveredLines += coveredLines;
		totalCoverableLines += coverableLines;
	}

	auto coveragePercent = static_cast<unsigned int>(totalCoveredLines*HundredPercent / totalCoverableLines);

	GLib::Eval::Evaluator e;
	e.Add("title", testName);
	e.Add("time", time);
	e.Add("isRoot", true);
	e.Add("isChild", false); // todo !isRoot
	e.Add("styleSheet", "./coverage.css");
	e.Add("directories", directories);
	e.Add("coveredLines", totalCoveredLines);
	e.Add("coverableLines", totalCoverableLines);
	e.Add("coveragePercent", coveragePercent);
	e.Add("coverageStyle", CoverageLevel(coveragePercent));

	auto rootIndex = htmlPath / "index.html";
	std::ofstream out(rootIndex);
	if (!out)
	{
		throw std::runtime_error("Unable to create output file : " + rootIndex.u8string());
	}

	GLib::Html::Generate(e, dirTemplate, out);
}

void HtmlReport::GenerateIndices() const
{
	for (const auto & pathChildrenPair : index)
	{
		const auto& subPath = pathChildrenPair.first;
		const auto& children = pathChildrenPair.second;

		std::vector<Directory> directories;

		unsigned int totalCoveredLines{};
		unsigned int totalCoverableLines{};
		for (const FileCoverageData & data : children)
		{
			totalCoveredLines += data.CoveredLines();
			totalCoverableLines += data.CoverableLines();
		}
		auto coveragePercent = static_cast<unsigned int>(totalCoveredLines*HundredPercent / totalCoverableLines);

		for (const FileCoverageData & data : children)
		{
			std::string text = data.Path().filename().u8string();
			directories.emplace_back(text, text+".html", data.CoveredLines(), data.CoverableLines());
		}

		auto path = htmlPath / subPath;
		auto relativePath = std::filesystem::relative(htmlPath, path);
		auto css = (relativePath / "coverage.css").generic_u8string();

		GLib::Eval::Evaluator e;
		e.Add("title", testName);
		e.Add("time", time);
		e.Add("isRoot", false);
		e.Add("isChild", true); // todo !isRoot
		e.Add("index", (relativePath / "index.html").generic_u8string());
		e.Add("subPath", subPath.generic_u8string());
		e.Add("styleSheet", css);
		e.Add("directories", directories);
		e.Add("coveredLines", totalCoveredLines);
		e.Add("coverableLines", totalCoverableLines);
		e.Add("coveragePercent", coveragePercent);
		e.Add("coverageStyle", CoverageLevel(coveragePercent));

		auto pathIndex = path / "index.html";
		std::ofstream out(pathIndex);
		if (!out)
		{
			throw std::runtime_error("Unable to create output file : " + pathIndex.u8string());
		}

		GLib::Html::Generate(e, dirTemplate, out);
	}
}

void HtmlReport::GenerateSourceFile(std::filesystem::path & subPath, const FileCoverageData & data) const
{
	auto targetPath = (htmlPath / subPath);
	auto relativePath = std::filesystem::relative(htmlPath, targetPath.parent_path());

	const std::filesystem::path & sourceFile = data.Path();
	std::ifstream in(sourceFile);
	if (!in)
	{
		throw std::runtime_error("Unable to open input file : " + sourceFile.u8string());
	}

	const auto & lc = data.LineCoverage();

	std::ostringstream source;
	{
		std::stringstream buffer;
		buffer << in.rdbuf();
		Htmlify(std::string_view{buffer.str()}, source);
	}

	std::vector<Line> lines;

	for (auto sourceLine : GLib::Util::Splitter{source.str(), "\n"})
	{
		auto lineNumber = static_cast<unsigned int>(lines.size()+1);
		const char * style = "";
		auto it = lc.find(lineNumber);
		if (it != lc.end())
		{
			style = it->second == 0 ? "ncov" : "cov";
		}
		lines.push_back({sourceLine, {}, style});
	}

	auto maxLineNumberWidth = static_cast<unsigned int>(floor(log10(lines.size()))) + 1;
	for (size_t i = 0; i < lines.size(); ++i)
	{
		std::ostringstream paddedLineNumber;
		paddedLineNumber << std::setw(maxLineNumberWidth) << i + 1; // use a width format specifier in template?
		lines[i].number = paddedLineNumber.str();
	}

	GLib::Eval::Evaluator e;

	auto parent = subPath.parent_path();
	auto css = (relativePath / "coverage.css").generic_u8string();
	auto coveragePercent = static_cast<unsigned int>(data.CoveredLines()*HundredPercent / lc.size());

	e.Add("title", subPath.generic_u8string());
	e.Add("testName", testName);
	e.Add("time", time);
	e.Add("parent", parent.generic_u8string());
	e.Add("fileName", targetPath.filename().u8string());
	e.Add("styleSheet", css);
	e.Add("coverageStyle", CoverageLevel(coveragePercent));
	e.Add("coveredLines", data.CoveredLines());
	e.Add("coverableLines", lc.size());
	e.Add("coveragePercent", coveragePercent);
	e.Add("index", (relativePath / "index.html").generic_u8string());

	e.AddCollection("lines", lines);

	create_directories(targetPath.parent_path());
	std::ofstream out(targetPath.concat(L".html"));
	if(!out)
	{
		throw std::runtime_error("Unable to create file");
	}

	GLib::Html::Generate(e, fileTemplate, out);
}