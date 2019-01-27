#pragma once

#include <map>
#include <set>

class HtmlPrinter;

namespace std
{
	namespace filesystem
	{
		class path;
	}
}

struct FileCoverageData;

class HtmlReport
{
	const std::filesystem::path & htmlPath;
	std::set<std::filesystem::path> rootPaths;
	const std::filesystem::path & cssPath;

	std::map<std::filesystem::path, std::map<std::filesystem::path, unsigned int>> index;

public:
	HtmlReport(const std::string & title, const std::filesystem::path & htmlPath,
		const std::map<std::filesystem::path, FileCoverageData> & fileCoverageData);

private:
	void Create(const std::filesystem::path & sourcePath, const FileCoverageData & data);

	void GenerateSourceFile(const std::filesystem::path & sourceFile, const std::filesystem::path & destFile, const std::map<unsigned int, size_t> & lines,
		const std::string & title, unsigned int coveragePercent) const;

	void GenerateIndices(const std::string & title) const;

	static std::filesystem::path Initialise(const std::filesystem::path & path);
	static void CreateCssFile(const std::filesystem::path & path);
	static std::set<std::filesystem::path> RootPaths(const std::map<std::filesystem::path, FileCoverageData> & data);
	static void AddHtmlCoverageBar(HtmlPrinter & printer, unsigned int percent);
};
