#pragma once

#include "CoverageLevel.h"

#include <GLib/Eval/Value.h>

#include <string>
#include <utility>

class Directory
{
	std::string name, link;
	unsigned int coveredLines;
	unsigned int coverableLines;
	unsigned int minCoveragePrecent;
	unsigned int coveredFunctions;
	unsigned int coverableFunctions;

public:
	Directory(std::string name, std::string link, unsigned int coveredLines, unsigned int coverableLines, unsigned int minCoveragePrecent,
		unsigned int coveredFunctions, unsigned int coverableFunctions)
		: name(move(name))
		, link(move(link))
		, coveredLines(coveredLines)
		, coverableLines(coverableLines)
		, minCoveragePrecent(minCoveragePrecent)
		, coveredFunctions(coveredFunctions)
		, coverableFunctions(coverableFunctions)
	{}

	std::string Name() const
	{
		return name;
	}

	std::string Link() const
	{
		return link;
	}

	enum CoverageLevel Style() const
	{
		return CoverageLevel(CoveragePercent());
	}

	unsigned int CoveragePercent() const
	{
		return Percentage(coveredLines,  coverableLines);
	}

	unsigned int CoveredLines() const
	{
		return coveredLines;
	}

	unsigned int CoverableLines() const
	{
		return coverableLines;
	}

	unsigned int MinCoveragePercent() const
	{
		return minCoveragePrecent;
	}

	enum CoverageLevel MinCoverageStyle() const
	{
		return CoverageLevel(minCoveragePrecent);
	}

	unsigned int CoveredFunctions() const
	{
		return coveredFunctions;
	}

	unsigned int CoverableFunctions() const
	{
		return coverableFunctions;
	}

	unsigned int CoveredFunctionsPercent() const
	{
		return Percentage(coveredFunctions, coverableFunctions);
	}
};

template <>
struct GLib::Eval::Visitor<Directory>
{
	static void Visit(const Directory & dir, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "name")
		{
			return f(Value(dir.Name()));
		}

		if (propertyName == "link")
		{
			return f(Value(dir.Link()));
		}

		if (propertyName == "coveragePercent")
		{
			return f(Value(dir.CoveragePercent()));
		}

		if (propertyName == "coveredLines")
		{
			return f(Value(dir.CoveredLines()));
		}

		if (propertyName == "coverableLines")
		{
			return f(Value(dir.CoverableLines()));
		}

		if (propertyName == "coverageStyle")
		{
			return f(Value(dir.Style()));
		}

		if (propertyName == "minCoveragePercent")
		{
			return f(Value(dir.MinCoveragePercent()));
		}

		if (propertyName == "minCoverageStyle")
		{
			return f(Value(dir.MinCoverageStyle()));
		}

		if (propertyName == "coveredFunctions")
		{
			return f(Value(dir.CoveredFunctions()));
		}

		if (propertyName == "coverableFunctions")
		{
			return f(Value(dir.CoverableFunctions()));
		}

		if (propertyName == "coveredFunctionsPercent")
		{
			return f(Value(dir.CoveredFunctionsPercent()));
		}

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};
