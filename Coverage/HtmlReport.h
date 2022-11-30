#pragma once

#include "Types.h"

#include <GLib/Flogging.h>

#include <list>

class FileCoverageData;

class HtmlReport
{
	inline static auto const log = GLib::Flog::LogManager::GetLog<HtmlReport>();

	std::string testName;
	std::string time;
	std::filesystem::path const & htmlPath;
	std::set<std::filesystem::path> rootPaths;
	std::filesystem::path cssPath;
	std::string rootTemplate;
	std::string dirTemplate;
	std::string fileTemplate;
	std::string functionsTemplate;
	std::map<std::filesystem::path, std::list<FileCoverageData>> index;
	bool showWhiteSpace;

public:
	HtmlReport(std::string testName, std::filesystem::path const & htmlPath, CoverageData const & coverageData, bool showWhiteSpace);

private:
	void GenerateRootIndex() const;
	void GenerateIndices() const;
	void GenerateSourceFile(std::filesystem::path const & subPath, FileCoverageData const & data) const;

	static std::filesystem::path Initialise(std::filesystem::path const & path);
	static std::set<std::filesystem::path> RootPaths(CoverageData const & data);
};
