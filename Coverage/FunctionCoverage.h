#pragma once

class FunctionCoverage
{
	unsigned int id;
	std::string nameSpace;
	std::string className;
	std::string functionName;
	unsigned int line;
	unsigned int coveredLines;
	unsigned int coverableLines;

public:
	FunctionCoverage(unsigned int id, std::string nameSpace, std::string className, std::string functionName, unsigned int line,
		unsigned int coveredLines, unsigned int coverableLines)
		: id{id}
		, nameSpace{std::move(nameSpace)}
		, className{std::move(className)}
		, functionName{std::move(functionName)}
		, line{line}
		, coveredLines{coveredLines}
		, coverableLines{coverableLines}
	{}

	const unsigned int & Id() const { return id; }
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
	// id at end to make unique, is ok? do they still need merging?
	return
		std::tie(f1.Line(), f1.NameSpace(), f1.ClassName(), f1.FunctionName(), f1.Id()) <
		std::tie(f2.Line(), f2.NameSpace(), f2.ClassName(), f2.FunctionName(), f2.Id());

	/*return f1.Line() < f2.Line() &&
		f1.NameSpace() < f2.NameSpace() &&
		f1.ClassName() < f2.ClassName() && 
		f1.FunctionName() < f2.FunctionName() &&
		f1.Id() < f2.Id();*/
}

