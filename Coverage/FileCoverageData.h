#pragma once

#include "Function.h"

#include <filesystem>
#include <map>
#include <utility>

using LineCoverage = std::map<unsigned int, unsigned int>;
using Functions = std::map<unsigned int, Function>;
/* use struct node_hash?
{
	std::size_t operator()(const Function & f) const
	{
		return std::hash<unsigned int>()(f.Id());
	}
}*/

class FileCoverageData
{
	std::filesystem::path const path;
	unsigned int coveredLines;
	LineCoverage lineCoverage;

	Functions functions;

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
		auto it = functions.find(function.Id());
		if (it != functions.end())
		{
			throw std::runtime_error("unexpected");
			//it->Accumulate(function);
		}
		else
		{
			functions.emplace(function.Id(), function);
		}
	}

	const Functions & Functions() const
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
		for (const auto & [id, f] : functions) // improve
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

	const LineCoverage & LineCoverage() const
	{
		return lineCoverage;
	}
};

