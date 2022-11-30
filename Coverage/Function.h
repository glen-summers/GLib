#pragma once

#include <ranges>

#include "Address.h"
#include "Types.h"

#include <string>
#include <tuple>

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

	std::string const & NameSpace() const
	{
		return nameSpace;
	}

	std::string const & ClassName() const
	{
		return className;
	}

	std::string const & FunctionName() const
	{
		return functionName;
	}

	FileLines const & FileLines() const
	{
		return fileLines;
	}

	// called when another address seen for the same function symbolId
	// 1. lines in same function
	// 2. lines merged into constructor from in class assignments, can cause multiple file names per accumulated address
	void Accumulate(Address const & address) const
	{
		for (auto const & [file, addressLines] : address.FileLines())
		{
			for (auto const & line : addressLines | std::views::keys) // map merge method?
			{
				fileLines[file][line] |= address.Visited();
			}
		}
	}

	bool Merge(Function const & added, std::filesystem::path const & path) const
	{
		auto const addedIt = added.fileLines.find(path);
		auto const existingIt = fileLines.find(path);

		if (existingIt == fileLines.end() || addedIt == added.fileLines.end())
		{
			return false;
		}

		bool merged {};
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

	static bool Overlap(Lines const & lines1, Lines const & lines2)
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
		size_t total {};
		for (auto const & lines : fileLines | std::views::values)
		{
			for (auto const & isCovered : lines | std::views::values)
			{
				// improve? keep tally?
				if (isCovered)
				{
					++total;
				}
			}
		}
		return total;
	}

	size_t AllLines() const
	{
		size_t total {};
		for (auto const & lines : fileLines | std::views::values)
		{
			total += lines.size();
		}
		return total;
	}
};

inline bool operator<(Function const & f1, Function const & f2)
{
	return std::tie(f1.NameSpace(), f1.ClassName(), f1.FunctionName()) < std::tie(f2.NameSpace(), f2.ClassName(), f2.FunctionName());
}
