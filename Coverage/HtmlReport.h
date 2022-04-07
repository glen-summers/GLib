#pragma once

#include "Types.h"

#include <GLib/Flogging.h>

#include <list>

class FileCoverageData;

class HtmlReport
{
	inline static const auto log = GLib::Flog::LogManager::GetLog<HtmlReport>();

	std::string testName;
	std::string time;
	const std::filesystem::path & htmlPath;
	std::set<std::filesystem::path> rootPaths;
	std::filesystem::path cssPath;
	std::string rootTemplate;
	std::string dirTemplate;
	std::string fileTemplate;
	std::string functionsTemplate;
	std::map<std::filesystem::path, std::list<FileCoverageData>> index;
	bool showWhiteSpace;

public:
	HtmlReport(std::string testName, const std::filesystem::path & htmlPath, const CoverageData & coverageData, bool showWhiteSpace);

private:
	void GenerateRootIndex() const;
	void GenerateIndices() const;
	void GenerateSourceFile(const std::filesystem::path & subPath, const FileCoverageData & data) const;

	static std::filesystem::path Initialise(const std::filesystem::path & path);
	static std::set<std::filesystem::path> RootPaths(const CoverageData & data);
};
