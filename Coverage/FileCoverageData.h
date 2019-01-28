#pragma once

#include <map>
#include <filesystem>
#include <utility>

class FileCoverageData
{
	std::filesystem::path const path;
	unsigned int coveredLines;
	std::map<unsigned int, unsigned int> lineCoverage;

	public:
		FileCoverageData(std::filesystem::path path)
			: path(std::move(path))
			, coveredLines()
		{}

		void AddLine(unsigned int line, bool covered)
		{
			size_t const hitCount = lineCoverage[line] += covered ? 1 : 0;
			if (covered && hitCount == 1)
			{
				++coveredLines;
			}
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

		const std::map<unsigned int, unsigned int> & LineCoverage() const
		{
			return lineCoverage;
		}

	
};

