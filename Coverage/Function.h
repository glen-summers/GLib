#pragma once

#include <ranges>
#include <string>
#include <tuple>

#include <GLib/Cvt.h>

#include "Address.h"
#include "Types.h"

class Function
{
	std::string nameSpace;
	std::string className;
	std::string functionName;

public:
	Function(std::string nameSpace, std::string className, std::string functionName)
		: nameSpace(std::move(nameSpace))
		, className(std::move(className))
		, functionName(std::move(functionName))
	{}

	static FileLines & GetFileLines() noexcept
	{
		try
		{
			static FileLines fileLines;
			return fileLines;
		}
		catch (std::exception &)
		{
			std::terminate();
		}
	}

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

	// called when another address seen for the same function symbolId
	// 1. lines in same function
	// 2. lines merged into constructor from in class assignments, can cause multiple file names per accumulated address
	void Accumulate(Address const & address) const
	{
		for (auto & [file, addressLines] : address.GetFileLines())
		{
			for (auto & line : addressLines | std::views::keys) // map merge method?
			{
				GetFileLines()[file][line] |= address.Visited();
			}
		}
	}

	[[nodiscard]] bool Merge(Function const & added, std::filesystem::path const & path) const
	{
		auto const addedIt = added.GetFileLines().find(path);
		auto const existingIt = GetFileLines().find(path);

		if (existingIt == GetFileLines().end() || addedIt == added.GetFileLines().end())
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

	[[nodiscard]] size_t CoveredLines() const
	{
		size_t total {};
		for (auto const & lines : GetFileLines() | std::views::values)
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

	[[nodiscard]] size_t AllLines() const
	{
		size_t total {};
		for (auto const & lines : GetFileLines() | std::views::values)
		{
			total += lines.size();
		}
		return total;
	}
};

inline bool operator<(Function const & function1, Function const & function2)
{
	return std::tie(function1.NameSpace(), function1.ClassName(), function1.FunctionName()) <
				 std::tie(function2.NameSpace(), function2.ClassName(), function2.FunctionName());
}
