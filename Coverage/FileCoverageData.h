#pragma once

#include "Function.h"

#include <filesystem>
#include <set>
#include <unordered_map>
#include <utility>

using LineCoverage = std::unordered_map<unsigned int, unsigned int>;
using Functions = std::multiset<Function>;

class FileCoverageData
{
	std::filesystem::path const path;
	unsigned int coveredLines {};
	LineCoverage lineCoverage;

	Functions functions;

public:
	explicit FileCoverageData(std::filesystem::path path);

	void AddLine(unsigned int line, bool covered);

	void AddFunction(Function const & addedFunction);

	[[nodiscard]] Functions const & Functions() const;

	[[nodiscard]] std::filesystem::path const & Path() const;

	[[nodiscard]] unsigned int CoveredLines() const;

	[[nodiscard]] unsigned int CoverableLines() const;

	[[nodiscard]] unsigned int CoveredFunctions() const;

	[[nodiscard]] unsigned int CoverableFunctions() const;

	[[nodiscard]] LineCoverage const & LineCoverage() const;
};
