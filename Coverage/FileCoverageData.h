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
	explicit FileCoverageData(std::filesystem::path path)
		: path(std::move(path))
	{}

	void AddLine(unsigned int const line, bool const covered)
	{
		unsigned int & hitCount = lineCoverage[line];
		if (covered && ++hitCount == 1)
		{
			++coveredLines;
		}
	}

	void AddFunction(Function const & addedFunction)
	{
		bool accumulated {};
		auto const lowerIt = functions.lower_bound(addedFunction);
		if (lowerIt != functions.end())
		{
			auto const upperIt = functions.upper_bound(addedFunction);
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

	[[nodiscard]] Functions const & Functions() const
	{
		return functions;
	}

	[[nodiscard]] std::filesystem::path const & Path() const
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
		for (auto const & function : functions) // improve
		{
			static_cast<void>(function);
			if (function.CoveredLines() != 0)
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

	[[nodiscard]] LineCoverage const & LineCoverage() const
	{
		return lineCoverage;
	}
};
