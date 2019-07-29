#include "pch.h"

#include "HtmlReport.h"
#include "FileCoverageData.h"
#include "RootDirs.h"
#include "Directory.h"

#include "resource.h"

#include "GLib/formatter.h"
#include "GLib/Win/Resources.h"
#include "GLib/Eval/TemplateEngine.h"
#include "GLib/Eval/Evaluator.h"

#include <fstream>
#include <filesystem>
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
		if (propertyName == "style") return f(Value(line.style));
		if (propertyName == "number") return f(Value(line.number));
		if (propertyName == "text") return f(Value(line.text));
		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};

HtmlReport::HtmlReport(const std::string & title, const std::filesystem::path & htmlPath,
	const std::map<std::filesystem::path, FileCoverageData> & fileCoverageData)
	: htmlPath(htmlPath)
	, rootPaths(RootPaths(fileCoverageData))
	, cssPath(Initialise(htmlPath))
	, dirTemplate(GLib::Win::LoadResourceString(nullptr, IDR_DIRECTORY, RT_HTML))
	, fileTemplate(GLib::Win::LoadResourceString(nullptr, IDR_FILE, RT_HTML))
{
	for (const auto & fileDataPair : fileCoverageData)
	{
		const FileCoverageData & data = fileDataPair.second;

		std::filesystem::path subPath = Reduce(data.Path(), rootPaths);
		std::filesystem::path targetPath = (htmlPath / subPath).concat(L".html");
		GenerateSourceFile(targetPath, "Coverage - " + subPath.u8string(), data);

		index[subPath.parent_path()].push_back(data);
	}
	GenerateRootIndex(title);
	GenerateIndices(title);
}

std::filesystem::path HtmlReport::Initialise(const std::filesystem::path & path)
{
	std::string s = GLib::Win::LoadResourceString(nullptr, IDR_STYLESHEET, RT_HTML);

	remove_all(path);
	create_directories(path);
	auto cssPath = path / "coverage.css";
	std::ofstream css(cssPath);
	if(!css)
	{
		throw std::runtime_error("Unable to create file");
	}
	css << s;
	return cssPath;
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

// move

void HtmlReport::GenerateRootIndex(const std::string & title) const
{
	std::vector<Directory> directories;
	for (const auto & pathChildrenPair : index)
	{
		const auto& name = pathChildrenPair.first;
		const auto& link = name / "index.html";
		const auto& children = pathChildrenPair.second;

		unsigned int totalCoveredLines{}, totalCoverableLines{};
		for (const FileCoverageData & data : children)
		{
			totalCoveredLines += data.CoveredLines();
			totalCoverableLines += data.CoverableLines();
		}
		directories.emplace_back(name.u8string(), link.generic_u8string(), totalCoveredLines, totalCoverableLines);
	}

	GLib::Eval::Evaluator e;
	e.Add("title", title);
	e.Add("styleSheet", "./coverage.css");
	e.Add("directories", directories);

	auto rootIndex = htmlPath / "index.html";
	std::ofstream out(rootIndex);
	if (!out)
	{
		throw std::runtime_error("Unable to create output file : " + rootIndex.u8string());
	}

	GLib::Eval::TemplateEngine::Generate(e, dirTemplate, out);
}

void HtmlReport::GenerateIndices(const std::string & title) const
{
	for (const auto & pathChildrenPair : index)
	{
		const auto& relativePath = pathChildrenPair.first;
		const auto& children = pathChildrenPair.second;

		std::vector<Directory> directories;

		unsigned int totalCoveredLines{}, totalCoverableLines{};
		for (const FileCoverageData & data : children)
		{
			totalCoveredLines += data.CoveredLines();
			totalCoverableLines += data.CoverableLines();
		}

		for (const FileCoverageData & data : children)
		{
			std::string text = data.Path().filename().u8string();
			directories.emplace_back(text, text+".html", data.CoveredLines(), data.CoverableLines());
		}

		auto path = htmlPath / relativePath;
		auto css = (std::filesystem::relative(htmlPath, path) / "coverage.css").generic_u8string();
		auto pathIndex = path / "index.html";

		GLib::Eval::Evaluator e;
		e.Add("title", title);
		e.Add("styleSheet", css);
		e.Add("directories", directories);

		std::ofstream out(pathIndex);
		if (!out)
		{
			throw std::runtime_error("Unable to create output file : " + pathIndex.u8string());
		}

		GLib::Eval::TemplateEngine::Generate(e, dirTemplate, out);
	}
}

void HtmlReport::GenerateSourceFile(std::filesystem::path & path, const std::string & title, const FileCoverageData & data) const
{
	const std::filesystem::path & sourceFile = data.Path();
	std::ifstream in(sourceFile);
	if (!in)
	{
		throw std::runtime_error("Unable to open input file : " + sourceFile.u8string());
	}

	const auto & lc = data.LineCoverage();
	std::vector<Line> lines;
	while (!in.eof())
	{
		std::string s;
		std::getline(in, s);
		auto line = static_cast<unsigned int>(lines.size()+1);
		const char * style = "";
		auto it = lc.find(line);
		if (it != lc.end())
		{
			style = it->second == 0 ? "ncov" : "cov";
		}
		std::ostringstream l;
		l << std::setw(6) << line; // use format specifier
		lines.push_back({move(s), l.str(), style});
	}

	GLib::Eval::Evaluator e;

	auto css = (relative(htmlPath, path.parent_path()) / "coverage.css").generic_u8string();
	auto coveragePercent = static_cast<unsigned int>(data.CoveredLines()*100 / lc.size());

	e.Add("styleSheet", css);
	e.Add("title", title);
	e.Add("coverageStyle", CoverageStyle(coveragePercent));
	e.Add("coveredLines", data.CoveredLines());
	e.Add("coverableLines", lc.size());
	e.Add("coveragePercent", coveragePercent);

	e.AddCollection("lines", lines);

	create_directories(path.parent_path());
	std::ofstream out(path);
	if(!out)
	{
		throw std::runtime_error("Unable to create file");
	}

	GLib::Eval::TemplateEngine::Generate(e, fileTemplate, out);
}