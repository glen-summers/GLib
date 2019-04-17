#pragma once

#include <map>
#include <set>
#include <list>
#include <string>

class HtmlPrinter;

namespace std
{
	namespace filesystem
	{
		class path;
	}
}

class FileCoverageData;

class HtmlReport
{
	const std::filesystem::path & htmlPath;
	std::set<std::filesystem::path> rootPaths;
	const std::filesystem::path & cssPath;

	std::map<std::filesystem::path, std::list<FileCoverageData>> index;

public:
	HtmlReport(const std::string & title, const std::filesystem::path & htmlPath,
		const std::map<std::filesystem::path, FileCoverageData> & fileCoverageData);

private:
	void GenerateSourceFile(const std::filesystem::path & destFile, const std::string & title, const FileCoverageData & data) const;

	void GenerateIndices(const std::string & title) const;

	static std::filesystem::path Initialise(const std::filesystem::path & path);
	static void CreateCssFile(const std::filesystem::path & path);
	static std::set<std::filesystem::path> RootPaths(const std::map<std::filesystem::path, FileCoverageData> & data);
	static void AddHtmlCoverageBar(HtmlPrinter & printer, unsigned int percent);
};
