#include "pch.h"

#include "FileCoverageData.h"

FileCoverageData::FileCoverageData(std::filesystem::path path)
	: path(std::move(path))
{}

void FileCoverageData::AddLine(unsigned int const line, bool const covered)
{
	unsigned int & hitCount = lineCoverage[line];
	if (covered && ++hitCount == 1)
	{
		++coveredLines;
	}
}

void FileCoverageData::AddFunction(Function const & addedFunction)
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

Functions const & FileCoverageData::Functions() const
{
	return functions;
}

std::filesystem::path const & FileCoverageData::Path() const
{
	return path;
}

unsigned int FileCoverageData::CoveredLines() const
{
	return coveredLines;
}

unsigned int FileCoverageData::CoverableLines() const
{
	return static_cast<unsigned int>(lineCoverage.size());
}

unsigned int FileCoverageData::CoveredFunctions() const
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

unsigned int FileCoverageData::CoverableFunctions() const
{
	return static_cast<unsigned int>(functions.size());
}

LineCoverage const & FileCoverageData::LineCoverage() const
{
	return lineCoverage;
}
