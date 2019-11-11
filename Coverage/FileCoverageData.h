#pragma once

#include "Function.h"

#include <filesystem>
#include <map>
#include <utility>

class FileCoverageData
{
	std::filesystem::path const path;
	unsigned int coveredLines;
	std::map<unsigned int, unsigned int> lineCoverage;
	std::set<Function> functions;

public:
	FileCoverageData(std::filesystem::path path)
		: path(std::move(path))
		, coveredLines()
	{}

	void AddLine(unsigned int line, bool covered)
	{
		unsigned int & hitCount = lineCoverage[line];
		if (covered && ++hitCount == 1)
		{
			++coveredLines;
		}
	}

	void AddFunction(const Function & function)
	{
		auto it = functions.find(function);
		if (it != functions.end())
		{
			it->Accumulate(function);
		}
		else
		{
			functions.insert(function);
		}
	}

	const std::set<Function> & Functions() const
	{
		return functions;
	}

	const std::filesystem::path & Path() const
	{
		return path;
	}

	unsigned int CoveredLines() const
	{
		return coveredLines;
	}

	unsigned int CoverableLines() const
	{
		return static_cast<unsigned int>(lineCoverage.size());
	}

	unsigned int CoveredFunctions() const
	{
		unsigned int value{};
		for (const auto & f : functions) // improve
		{
			if (f.CoveredLines() != 0)
			{
				++value;
			}
		}
		return value;
	}

	unsigned int CoverableFunctions() const
	{
		return static_cast<unsigned int>(functions.size());
	}

	const std::map<unsigned int, unsigned int> & LineCoverage() const
	{
		return lineCoverage;
	}
};

