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
									 unsigned int const coverableLines)
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

inline bool operator<(FunctionCoverage const & function1, FunctionCoverage const & function2)
{
	// default sort by line, html has to resort itself
	return std::tie(function1.Line(), function1.NameSpace(), function1.ClassName(), function1.FunctionName()) <
				 std::tie(function2.Line(), function2.NameSpace(), function2.ClassName(), function2.FunctionName());
}
