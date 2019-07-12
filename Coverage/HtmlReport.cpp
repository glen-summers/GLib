#include "pch.h"

#include "HtmlReport.h"
#include "FileCoverageData.h"

#include "RootDirs.h"
#include "HtmlPrinter.h"

#include "GLib/formatter.h"

#include <fstream>
#include <filesystem>
#include <set>

namespace
{
	std::filesystem::path Append(std::filesystem::path p, const wchar_t * appendage)
	{
		return p.replace_filename(p.filename().wstring() + appendage);
	}
}

HtmlReport::HtmlReport(const std::string & title, const std::filesystem::path & htmlPath,
	const std::map<std::filesystem::path, FileCoverageData> & fileCoverageData)
	: htmlPath(htmlPath)
	, rootPaths(RootPaths(fileCoverageData))
	, cssPath(Initialise(htmlPath))
{
	for (const auto & fileDataPair : fileCoverageData)
	{
		const FileCoverageData & data = fileDataPair.second;

		std::filesystem::path subPath = Reduce(data.Path(), rootPaths);
		std::filesystem::path targetPath = (htmlPath / subPath).concat(L".html");
		GenerateSourceFile(targetPath, "Coverage - " + subPath.u8string(), data);

		index[subPath.parent_path()].push_back(data);
	}
	GenerateIndices(title);
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

td.coverageNumber {
	text-align: right;
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

void HtmlReport::GenerateSourceFile(const std::filesystem::path & destFile, const std::string & title,
	const FileCoverageData & data) const
{
	const std::filesystem::path & sourceFile = data.Path();
	const auto & lineCoverage = data.LineCoverage();
	auto coveragePercent = static_cast<unsigned int>(data.CoveredLines()*100 / lineCoverage.size());

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

	printer.OpenTable(1, 1, 0);
	printer.PushAttribute("class", "centre");
	printer.OpenElement("tr");
	printer.OpenElement("td");
	printer.PushText("Coverage");
	printer.CloseElement(); // td
	printer.OpenElement("td");
	printer.PushText("%");
	printer.CloseElement(); // td
	printer.OpenElement("td");
	printer.PushText("Covered lines");
	printer.CloseElement(); // td
	printer.CloseElement(); // tr

	printer.OpenElement("tr");
	printer.OpenElement("td");
	AddHtmlCoverageBar(printer, coveragePercent);
	printer.CloseElement(); // td
	printer.OpenElement("td");
	printer.PushText(GLib::Formatter::Format("{0} %", coveragePercent));
	printer.CloseElement(); // td
	printer.OpenElement("td");
	printer.PushAttribute("class", "coverageNumber");
	printer.PushText(GLib::Formatter::Format("{0} / {1}", data.CoveredLines(), data.CoverableLines()));
	printer.CloseElement(); // td
	printer.CloseElement(); // tr

	printer.CloseElement(); // table
	printer.LineBreak();

	printer.OpenElement("pre");
	printer.OpenTable();

	for (size_t fileLine = 1; fileLine <= fileLines.size(); ++fileLine)
	{
		auto it = lineCoverage.find(static_cast<unsigned int>(fileLine));
		const char* style = nullptr;
		if (it != lineCoverage.end())
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

	create_directories(destFile.parent_path());
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

	rootIndex.OpenTable(1, 1, 0);
	rootIndex.PushAttribute("class", "centre");
	rootIndex.OpenElement("tr");
	rootIndex.OpenElement("td");
	rootIndex.PushText("Directory");
	rootIndex.CloseElement(); // td
	rootIndex.OpenElement("td");
	rootIndex.PushText("Coverage");
	rootIndex.CloseElement(); // td
	rootIndex.OpenElement("td");
	rootIndex.PushText("%");
	rootIndex.CloseElement(); // td
	rootIndex.OpenElement("td");
	rootIndex.PushText("Covered lines");
	rootIndex.CloseElement(); // td
	rootIndex.CloseElement(); // tr

	for (const auto & pathChildrenPair : index)
	{
		const auto& relativePath = pathChildrenPair.first;
		const auto& children = pathChildrenPair.second;

		unsigned int totalCoveredLines{}, totalCoverableLines{};
		for (const FileCoverageData & data : children)
		{
			totalCoveredLines += data.CoveredLines();
			totalCoverableLines += data.CoverableLines();
		}
		auto totalCoveragePercent = 100 * totalCoveredLines / totalCoverableLines;

		rootIndex.OpenElement("tr");
		rootIndex.OpenElement("td");
		auto relativePathIndex = relativePath / "index.html";
		rootIndex.Anchor(relativePathIndex, relativePath.u8string());
		rootIndex.CloseElement(); // td
		rootIndex.OpenElement("td");
		AddHtmlCoverageBar(rootIndex, totalCoveragePercent);
		rootIndex.CloseElement(); // td
		rootIndex.OpenElement("td");
		rootIndex.PushText(GLib::Formatter::Format("{0} %", totalCoveragePercent));
		rootIndex.CloseElement(); // td
		rootIndex.OpenElement("td");
		rootIndex.PushAttribute("class", "coverageNumber");
		rootIndex.PushText(GLib::Formatter::Format("{0} / {1}", totalCoveredLines, totalCoverableLines));
		rootIndex.CloseElement(); // td
		rootIndex.CloseElement(); // tr

		HtmlPrinter childList("Coverage - " + title + " - " + relativePath.u8string(), cssPath);
		childList.LineBreak();
		childList.OpenTable();
		childList.PushAttribute("class", "centre");
		childList.OpenTable(1, 1, 0);
		childList.PushAttribute("class", "centre");
		childList.OpenElement("tr");
		childList.OpenElement("td");
		childList.PushText("SourceFile");
		childList.CloseElement(); // td
		childList.OpenElement("td");
		childList.PushText("Coverage");
		childList.CloseElement(); // td
		childList.OpenElement("td");
		childList.PushText("%");
		childList.CloseElement(); // td
		childList.OpenElement("td");
		childList.PushText("Covered lines");
		childList.CloseElement(); // td
		childList.CloseElement(); // tr

		for (const FileCoverageData & data : children)
		{
			std::filesystem::path fileName = data.Path().filename();
			auto text = fileName.u8string();
			fileName += L".html";
			auto coveragePercent = 100 * data.CoveredLines() / data.CoverableLines();

			childList.OpenElement("tr");
			childList.OpenElement("td");
			childList.Anchor(fileName, text);
			childList.CloseElement(); // td
			childList.OpenElement("td");
			AddHtmlCoverageBar(childList, coveragePercent);
			childList.CloseElement(); // td
			childList.OpenElement("td");
			childList.PushText(GLib::Formatter::Format("{0} %", coveragePercent));
			childList.CloseElement(); // td
			childList.OpenElement("td");
			childList.PushAttribute("class", "coverageNumber");
			childList.PushText(GLib::Formatter::Format("{0} / {1}", data.CoveredLines(), data.CoverableLines()));
			childList.CloseElement(); // td
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
