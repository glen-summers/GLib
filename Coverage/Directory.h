#pragma once

#include "GLib/Eval/Value.h"

#include "CoverageLevel.h"

#include <string>
#include <utility>

class Directory
{
	std::string name, link;
	unsigned int coveredLines;
	unsigned int coverableLines;

public:
	Directory(std::string name, std::string link, unsigned int coveredLines, unsigned int coverableLines)
		: name(move(name))
		, link(move(link))
		, coveredLines(coveredLines)
		, coverableLines(coverableLines)
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
		return HundredPercent * coveredLines / coverableLines;
	}

	unsigned int CoveredLines() const
	{
		return coveredLines;
	}

	unsigned int CoverableLines() const
	{
		return coverableLines;
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

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};
