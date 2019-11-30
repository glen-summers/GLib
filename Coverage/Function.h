#pragma once

#include "Address.h"
#include "Types.h"

#include <string>

class Function
{
	std::string nameSpace;
	std::string className;
	std::string functionName;

	FileLines mutable fileLines;

public:
	Function(std::string nameSpace, std::string className, std::string functionName)
		: nameSpace(std::move(nameSpace))
		, className(std::move(className))
		, functionName(std::move(functionName))
	{}

	const std::string & NameSpace() const { return nameSpace; }
	const std::string & ClassName() const { return className; }
	const std::string & FunctionName() const { return functionName; }

	const FileLines & FileLines() const
	{
		return fileLines;
	}

	// called when another address seen for the same function symbolId
	// 1. lines in same function
	// 2. lines merged into constructor from in class assignments, can cause multiple file names per accumulated address
	void Accumulate(const Address & address) const
	{
		bool const visited = address.Visited();
		for (const auto & fileLineIt : address.FileLines())
		{
			const std::map<unsigned int, bool> & lines = fileLineIt.second;
			std::map<unsigned, bool> & pairs = fileLines[fileLineIt.first];

			for (const auto & lineIt : lines) // map merge method?
			{
				pairs[lineIt.first] |= visited;
			}
		}
	}

	bool Merge(const Function & added, const std::filesystem::path & path) const
	{
		auto addedIt = added.fileLines.find(path);
		auto existingIt = fileLines.find(path);

		if (existingIt == fileLines.end() || addedIt == added.fileLines.end())
		{
			return false;
		}

		bool merged{};
		if (Overlap(existingIt->second, addedIt->second))
		{
			for (auto [line, covered] : addedIt->second)
			{
				existingIt->second[line] |= covered;
			}
			merged = true;
		}

		return merged;
	}

	static bool Overlap(const Lines & lines1, const Lines & lines2)
	{
		if (lines1.empty() || lines2.empty())
		{
			return false;
		}

		if (lines1.rbegin()->first < lines2.begin()->first)
		{
			return false;
		}

		if (lines2.rbegin()->first < lines1.begin()->first)
		{
			return false;
		}

		return true;
	}

	size_t CoveredLines() const
	{
		size_t total{};
		for (const auto & x : fileLines)
		{
			for (const auto & y : x.second)
			{
				// improve? keep tally?
				if (y.second)
				{
					++total;
				}
			}
		}
		return total;
	}

	size_t AllLines() const
	{
		size_t total{};
		for (const auto & x : fileLines)
		{
			total += x.second.size();
		}
		return total;
	}
};

inline bool operator < (const Function & f1, const Function & f2)
{
	return std::tie(f1.NameSpace(), f1.ClassName(), f1.FunctionName()) < std::tie(f2.NameSpace(), f2.ClassName(), f2.FunctionName());
}
