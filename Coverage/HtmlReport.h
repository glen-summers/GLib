#pragma once

#include <filesystem>
#include <list>
#include <map>
#include <set>
#include <string>

class FileCoverageData;

class HtmlReport
{
	const std::filesystem::path & htmlPath;
	std::set<std::filesystem::path> rootPaths;
	std::filesystem::path cssPath;
	std::string dirTemplate; // store compiled Nodes(ren)
	std::string fileTemplate;

	std::map<std::filesystem::path, std::list<FileCoverageData>> index;

public:
	HtmlReport(const std::string & title, const std::filesystem::path & htmlPath,
		const std::map<std::filesystem::path, FileCoverageData> & fileCoverageData);

private:
	void GenerateRootIndex(const std::string & title) const;
	void GenerateIndices(const std::string & title) const;
	void GenerateSourceFile(std::filesystem::path & path, const std::string & title, const FileCoverageData & data) const;

	static std::filesystem::path Initialise(const std::filesystem::path & path);
	static std::set<std::filesystem::path> RootPaths(const std::map<std::filesystem::path, FileCoverageData> & data);
};
