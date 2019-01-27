#include "pch.h"

#include "HtmlReport.h"
#include "FileCoverageData.h"

#include "RootDirs.h"
#include "HtmlPrinter.h"

#include <fstream>
#include <filesystem>
#include <set>

namespace
{
	std::filesystem::path Append(std::filesystem::path p, const wchar_t * appendage)
	{
		return p.replace_filename(p.filename().wstring() + appendage);
	}
	wchar_t HtmlExtension[] = L".html";
}

HtmlReport::HtmlReport(const std::string & title, const std::filesystem::path & htmlPath,
	const std::map<std::filesystem::path, FileCoverageData> & fileCoverageData)
	: htmlPath(htmlPath)
	, rootPaths(RootPaths(fileCoverageData))
	, cssPath(Initialise(htmlPath))
{
	for (const auto & fileDataPair : fileCoverageData)
	{
		const std::filesystem::path & fileName = fileDataPair.first;
		const FileCoverageData & data = fileDataPair.second;
		Create(fileName, data);
	}
	GenerateIndices(title);
}

void HtmlReport::Create(const std::filesystem::path & sourcePath, const FileCoverageData & data)
{
	std::filesystem::path subPath = Reduce(sourcePath, rootPaths);
	std::filesystem::path filePath = htmlPath / subPath;
	create_directories(filePath.parent_path());
	auto coveragePercent = static_cast<unsigned int>(100 * data.coveredLines / data.lineCoverage.size());
	GenerateSourceFile(sourcePath, Append(filePath, HtmlExtension), data.lineCoverage, "Coverage - " + subPath.u8string(), coveragePercent);
	index[subPath.parent_path()].insert({ filePath.filename(), coveragePercent });
}

std::filesystem::path HtmlReport::Initialise(const std::filesystem::path & path)
{
	remove_all(path);
	create_directories(path);
	auto css = path / "coverage.css";
	CreateCssFile(css);
	return css;
}

void HtmlReport::CreateCssFile(const std::filesystem::path & path)
{
	// just store in filesystem
	std::ofstream css(path);
	css << R"(body {
  color: #000000;
  background-color: #FFFFFF;
}

hr {
    display: block;
    height: 2px;
    border: 0;
    border-top: 1px solid blue;
    margin: 1em 0;
    padding: 0;
}

table.centre {
  margin: auto;
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
  width:100px; height:10px; border:1px solid #000;
	margin: auto;
}

div.red {
  height:100%; background-color: #CE1620;
}

div.amber {
  height:100%; background-color: #FF7518;
}

div.green {
  height:100%; background-color: #03C03C;
}
	)";
	css.close();
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

void HtmlReport::GenerateSourceFile(const std::filesystem::path & sourceFile, const std::filesystem::path & destFile, const std::map<unsigned int, size_t> & lines,
	const std::string & title, unsigned int coveragePercent) const
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

	HtmlPrinter printer(title, cssPath);
	printer.LineBreak();
	printer.OpenTable();
	printer.PushAttribute("width", "100%");
	printer.PushAttribute("class", "centre");

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
	printer.LineBreak();

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
	printer.CloseElement(); // table
	printer.LineBreak();
	printer.Close();

	std::ofstream out(destFile);
	out << printer.Xml();
	if (out.fail())
	{
		throw std::runtime_error("Unable to create output file : " + destFile.u8string());
	}
}

void HtmlReport::GenerateIndices(const std::string & title) const
{
	HtmlPrinter rootIndex("Coverage - " + title, cssPath);
	rootIndex.LineBreak();

	rootIndex.OpenTable();
	rootIndex.PushAttribute("class", "centre");
	for (const auto & pathChildrenPair : index)
	{
		const auto& relativePath = pathChildrenPair.first;
		const auto& children = pathChildrenPair.second;

		rootIndex.OpenElement("tr");
		rootIndex.OpenElement("td");
		auto relativePathIndex = relativePath / "index.html";
		rootIndex.Anchor(relativePathIndex, relativePath.u8string());
		rootIndex.CloseElement(false); // td
		rootIndex.CloseElement(); // tr

		HtmlPrinter childList("Coverage - " + title + " - " + relativePath.u8string(), cssPath);
		childList.LineBreak();
		childList.OpenTable();
		childList.PushAttribute("class", "centre");

		for (const auto& child : children)
		{
			auto fileName = child.first;
			
			unsigned int coveragePercent = child.second;

			childList.OpenElement("tr");
			childList.OpenElement("td");
			childList.Anchor(Append(fileName, HtmlExtension), fileName.u8string());
			childList.CloseElement(false); // td
			childList.OpenElement("td", false);
			AddHtmlCoverageBar(childList, coveragePercent);
			childList.CloseElement(false); // td
			childList.CloseElement(); // tr
		}
		childList.CloseElement(); // table
		childList.LineBreak();
		childList.Close();
		auto pathIndex = htmlPath / relativePathIndex;
		std::ofstream out(pathIndex);
		if (out.fail())
		{
			throw std::runtime_error("Unable to create output file : " + pathIndex.u8string());
		}
		out << childList.Xml();
	}
	rootIndex.CloseElement(); // table
	rootIndex.LineBreak();
	rootIndex.Close();

	auto indexFile = htmlPath / "index.html";
	std::ofstream out(indexFile);
	if (out.fail())
	{
		throw std::runtime_error("Unable to create output file : " + indexFile.u8string());
	}
	out << rootIndex.Xml();
}

void HtmlReport::AddHtmlCoverageBar(HtmlPrinter & printer, unsigned int percent)
{
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
