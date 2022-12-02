#pragma once

#include "Types.h"

#include <GLib/Flogging.h>

#include <list>

class FileCoverageData;

class HtmlReport
{
	inline static auto const log = GLib::Flog::LogManager::GetLog<HtmlReport>();

	std::string const testName;
	std::string const time;
	std::filesystem::path const & htmlPath;
	std::set<std::filesystem::path> const rootPaths;
	std::filesystem::path const cssPath;
	std::string const rootTemplate;
	std::string const dirTemplate;
	std::string const fileTemplate;
	std::string const functionsTemplate;
	std::map<std::filesystem::path, std::list<FileCoverageData>> index;
	bool const showWhiteSpace;

public:
	HtmlReport(std::string testName, std::filesystem::path const & htmlPath, CoverageData const & coverageData, bool showWhiteSpace);

private:
	void GenerateRootIndex() const;
	void GenerateIndices() const;
	void GenerateSourceFile(std::filesystem::path const & subPath, FileCoverageData const & data) const;

	static std::filesystem::path Initialise(std::filesystem::path const & path);
	static std::set<std::filesystem::path> RootPaths(CoverageData const & data);
};
