#pragma once

#include "Types.h"

#include <list>

class FileCoverageData;

class HtmlReport
{
	std::string testName;
	std::string time;
	const std::filesystem::path & htmlPath;
	std::set<std::filesystem::path> rootPaths;
	std::filesystem::path cssPath;
	std::string rootTemplate; // store compiled Nodes(ren)
	std::string dirTemplate;
	std::string fileTemplate;
	std::map<std::filesystem::path, std::list<FileCoverageData>> index;

public:
	HtmlReport(std::string testName, const std::filesystem::path & htmlPath, const CoverageData & coverage);

private:
	void GenerateRootIndex() const;
	void GenerateIndices() const;
	void GenerateSourceFile(std::filesystem::path & subPath, const FileCoverageData & data) const;

	static std::filesystem::path Initialise(const std::filesystem::path & path);
	static std::set<std::filesystem::path> RootPaths(const CoverageData & data);
};
