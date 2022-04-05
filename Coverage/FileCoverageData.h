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
	unsigned int coveredLines;
	LineCoverage lineCoverage;

	Functions functions;

public:
	explicit FileCoverageData(std::filesystem::path path)
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

	void AddFunction(const Function & addedFunction)
	{
		bool accumulated {};
		auto lowerIt = functions.lower_bound(addedFunction);
		if (lowerIt != functions.end())
		{
			auto upperIt = functions.upper_bound(addedFunction);
			for (auto it = lowerIt; it != upperIt; ++it)
			{
				if (it->Merge(addedFunction, path))
				{
					accumulated = true;
					break;
				}
			}
		}

		if (!accumulated)
		{
			functions.emplace(addedFunction);
		}
	}

	[[nodiscard]] const Functions & Functions() const
	{
		return functions;
	}

	[[nodiscard]] const std::filesystem::path & Path() const
	{
		return path;
	}

	[[nodiscard]] unsigned int CoveredLines() const
	{
		return coveredLines;
	}

	[[nodiscard]] unsigned int CoverableLines() const
	{
		return static_cast<unsigned int>(lineCoverage.size());
	}

	[[nodiscard]] unsigned int CoveredFunctions() const
	{
		unsigned int value {};
		for (const auto & f : functions) // improve
		{
			if (f.CoveredLines() != 0)
			{
				++value;
			}
		}
		return value;
	}

	[[nodiscard]] unsigned int CoverableFunctions() const
	{
		return static_cast<unsigned int>(functions.size());
	}

	[[nodiscard]] const LineCoverage & LineCoverage() const
	{
		return lineCoverage;
	}
};
