#pragma once

#include "GLib/Eval/Value.h"

#include <string>
#include <utility>

// move?
inline const char * CoverageStyle(unsigned int coveragePercent)
{
	const int lowValue = 70;
	const int highValue = 90;
	const char* badStyle = "red";
	const char* warnStyle = "amber";
	const char* goodStyle = "green";

	return coveragePercent < lowValue
		? badStyle : coveragePercent < highValue
		? warnStyle : goodStyle;
}

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

	const char * Style() const
	{
		return CoverageStyle(CoveragePercent());
	}

	unsigned int CoveragePercent() const
	{
		return 100 * coveredLines / coverableLines;
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
		if (propertyName == "name") return f(Value(dir.Name()));
		if (propertyName == "link") return f(Value(dir.Link()));
		if (propertyName == "coveragePercent") return f(Value(dir.CoveragePercent()));
		if (propertyName == "coveredLines") return f(Value(dir.CoveredLines()));
		if (propertyName == "coverableLines") return f(Value(dir.CoverableLines()));
		if (propertyName == "coverageStyle") return f(Value(dir.Style()));
		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};
