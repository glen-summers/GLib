#pragma once

#include <string>
#include <tuple>

class FunctionCoverage
{
	std::string nameSpace;
	std::string className;
	std::string functionName;
	unsigned int line;
	unsigned int coveredLines;
	unsigned int coverableLines;

public:
	FunctionCoverage(std::string nameSpace, std::string className, std::string functionName, unsigned int const line, unsigned int const coveredLines,
									 unsigned int coverableLines)
		: nameSpace {std::move(nameSpace)}
		, className {std::move(className)}
		, functionName {std::move(functionName)}
		, line {line}
		, coveredLines {coveredLines}
		, coverableLines {coverableLines}
	{}

	[[nodiscard]] std::string const & NameSpace() const
	{
		return nameSpace;
	}

	[[nodiscard]] std::string const & ClassName() const
	{
		return className;
	}

	[[nodiscard]] std::string const & FunctionName() const
	{
		return functionName;
	}

	[[nodiscard]] unsigned int const & Line() const
	{
		return line;
	}

	[[nodiscard]] unsigned int CoveredLines() const
	{
		return coveredLines;
	}

	[[nodiscard]] unsigned int CoverableLines() const
	{
		return coverableLines;
	}
};

inline bool operator<(FunctionCoverage const & f1, FunctionCoverage const & f2)
{
	// default sort by line, html has to resort itself
	return std::tie(f1.Line(), f1.NameSpace(), f1.ClassName(), f1.FunctionName()) <
				 std::tie(f2.Line(), f2.NameSpace(), f2.ClassName(), f2.FunctionName());
}
