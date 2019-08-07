#pragma once

#include "Types.h"

class Address
{
	unsigned char oldData;
	FileLines fileLines; // use sparse container?
	bool visited;

public:
	explicit Address(unsigned char oldData) : oldData(oldData), visited()
	{}

	unsigned char OldData() const
	{
		return oldData;
	}

	const FileLines & FileLines() const
	{
		return fileLines;
	}

	bool Visited() const
	{
		return visited;
	}

	void Visit()
	{
		visited = true;
	}

	// multiple lines can be returned for the same address
	// 0. not seen multiple files returned but code is defensive just in case
	// 1. release build of {return atomicType++;} returns 3 lines for same address
	void AddFileLine(const std::wstring & fileName, unsigned int lineNumber)
	{
		fileLines[fileName].emplace(lineNumber, false);
	}
};

