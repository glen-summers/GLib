#pragma once

#include "CoverageLevel.h"

#include <GLib/Eval/Value.h>

#include <string>
#include <utility>

class Directory
{
	std::string name;
	std::string link;
	unsigned int const coveredLines;
	unsigned int const coverableLines;
	unsigned int const minCoveragePercent;
	unsigned int const coveredFunctions;
	unsigned int const coverableFunctions;

public:
	Directory(std::string name, std::string link, unsigned int const coveredLines, unsigned int const coverableLines,
						unsigned int const minCoveragePercent, unsigned int const coveredFunctions, unsigned int const coverableFunctions)
		: name(std::move(name))
		, link(std::move(link))
		, coveredLines(coveredLines)
		, coverableLines(coverableLines)
		, minCoveragePercent(minCoveragePercent)
		, coveredFunctions(coveredFunctions)
		, coverableFunctions(coverableFunctions)
	{}

	[[nodiscard]] std::string Name() const
	{
		return name;
	}

	[[nodiscard]] std::string Link() const
	{
		return link;
	}

	[[nodiscard]] CoverageLevel Style() const
	{
		return GetCoverageLevel(CoveragePercent());
	}

	[[nodiscard]] unsigned int CoveragePercent() const
	{
		return Percentage(coveredLines, coverableLines);
	}

	[[nodiscard]] unsigned int CoveredLines() const
	{
		return coveredLines;
	}

	[[nodiscard]] unsigned int CoverableLines() const
	{
		return coverableLines;
	}

	[[nodiscard]] unsigned int MinCoveragePercent() const
	{
		return minCoveragePercent;
	}

	[[nodiscard]] CoverageLevel MinCoverageStyle() const
	{
		return GetCoverageLevel(minCoveragePercent);
	}

	[[nodiscard]] unsigned int CoveredFunctions() const
	{
		return coveredFunctions;
	}

	[[nodiscard]] unsigned int CoverableFunctions() const
	{
		return coverableFunctions;
	}

	[[nodiscard]] unsigned int CoveredFunctionsPercent() const
	{
		return Percentage(coveredFunctions, coverableFunctions);
	}
};

template <>
struct GLib::Eval::Visitor<Directory>
{
	static void Visit(Directory const & dir, std::string const & propertyName, ValueVisitor const & visitor)
	{
		if (propertyName == "name")
		{
			return visitor(Value(dir.Name()));
		}

		if (propertyName == "link")
		{
			return visitor(Value(dir.Link()));
		}

		if (propertyName == "coveragePercent")
		{
			return visitor(Value(dir.CoveragePercent()));
		}

		if (propertyName == "coveredLines")
		{
			return visitor(Value(dir.CoveredLines()));
		}

		if (propertyName == "coverableLines")
		{
			return visitor(Value(dir.CoverableLines()));
		}

		if (propertyName == "coverageStyle")
		{
			return visitor(Value(dir.Style()));
		}

		if (propertyName == "minCoveragePercent")
		{
			return visitor(Value(dir.MinCoveragePercent()));
		}

		if (propertyName == "minCoverageStyle")
		{
			return visitor(Value(dir.MinCoverageStyle()));
		}

		if (propertyName == "coveredFunctions")
		{
			return visitor(Value(dir.CoveredFunctions()));
		}

		if (propertyName == "coverableFunctions")
		{
			return visitor(Value(dir.CoverableFunctions()));
		}

		if (propertyName == "coveredFunctionsPercent")
		{
			return visitor(Value(dir.CoveredFunctionsPercent()));
		}

		throw std::runtime_error("Unknown property : '" + propertyName + '\'');
	}
};
