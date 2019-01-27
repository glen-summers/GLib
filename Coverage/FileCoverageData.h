#pragma once

#include <map>

struct FileCoverageData
{
	size_t coveredLines{};
	std::map<unsigned int, size_t> lineCoverage;
};
