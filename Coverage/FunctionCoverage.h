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
	FunctionCoverage(std::string nameSpace, std::string className, std::string functionName, unsigned int line,
		unsigned int coveredLines, unsigned int coverableLines)
		: nameSpace{std::move(nameSpace)}
		, className{std::move(className)}
		, functionName{std::move(functionName)}
		, line{line}
		, coveredLines{coveredLines}
		, coverableLines{coverableLines}
	{}

	const std::string & NameSpace() const { return nameSpace; }
	const std::string & ClassName() const { return className; }
	const std::string & FunctionName() const { return functionName; }
	const unsigned int & Line() const { return line; }
	unsigned int CoveredLines() const { return coveredLines; }
	unsigned int CoverableLines() const { return coverableLines; }
};

inline bool operator < (const FunctionCoverage & f1, const FunctionCoverage & f2)
{
	// default sort by line, html has to resort itself
	return
		std::tie(f1.Line(), f1.NameSpace(), f1.ClassName(), f1.FunctionName()) <
		std::tie(f2.Line(), f2.NameSpace(), f2.ClassName(), f2.FunctionName());
}

