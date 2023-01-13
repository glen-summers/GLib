#pragma once

#include <ranges>
#include <string>
#include <tuple>

#include <GLib/Cvt.h>

#include "Address.h"
#include "Types.h"

class Function // NOLINT(bugprone-exception-escape)
{
	std::string nameSpace;
	std::string className;
	std::string functionName;

	FileLines mutable fileLines;

public:
	Function(std::string nameSpace, std::string className, std::string functionName);

	std::string const & NameSpace() const;

	std::string const & ClassName() const;

	std::string const & FunctionName() const;

	FileLines const & FileLines() const;

	void Accumulate(Address const & address) const;

	bool Merge(Function const & added, std::filesystem::path const & path) const;

	static bool Overlap(Lines const & lines1, Lines const & lines2);

	size_t CoveredLines() const;

	size_t AllLines() const;
};

inline bool operator<(Function const & function1, Function const & function2)
{
	return std::tie(function1.NameSpace(), function1.ClassName(), function1.FunctionName()) <
				 std::tie(function2.NameSpace(), function2.ClassName(), function2.FunctionName());
}
