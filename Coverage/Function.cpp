#include "pch.h"

#include "Function.h"

Function::Function(std::string nameSpace, std::string className, std::string functionName)
	: nameSpace(std::move(nameSpace))
	, className(std::move(className))
	, functionName(std::move(functionName))
{}

std::string const & Function::NameSpace() const
{
	return nameSpace;
}

std::string const & Function::ClassName() const
{
	return className;
}

std::string const & Function::FunctionName() const
{
	return functionName;
}

FileLines const & Function::FileLines() const
{
	return fileLines;
}

// called when another address seen for the same function symbolId
// 1. lines in same function
// 2. lines merged into constructor from in class assignments, can cause multiple file names per accumulated address
void Function::Accumulate(Address const & address) const
{
	for (auto const & [file, addressLines] : address.FileLines())
	{
		for (auto const & line : addressLines | std::views::keys) // map merge method?
		{
			fileLines[file][line] |= address.Visited();
		}
	}
}

bool Function::Merge(Function const & added, std::filesystem::path const & path) const
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
		for (auto const [line, covered] : addedIt->second)
		{
			existingIt->second[line] |= covered;
		}
		merged = true;
	}

	return merged;
}

bool Function::Overlap(Lines const & lines1, Lines const & lines2)
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

size_t Function::CoveredLines() const
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

size_t Function::AllLines() const
{
	size_t total {};
	for (auto const & lines : fileLines | std::views::values)
	{
		total += lines.size();
	}
	return total;
}
