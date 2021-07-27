#pragma once

#include "Types.h"

class Address
{
	unsigned char oldData;
	unsigned int symbolId;

	mutable FileLines fileLines; // use sparse container?
	mutable bool visited;

public:
	explicit Address(unsigned char oldData, unsigned int symbolId)
		: oldData(oldData)
		, visited()
		, symbolId(symbolId)
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

	void Visit() const
	{
		visited = true;
	}

	unsigned int SymbolId() const
	{
		return symbolId;
	}

	// multiple lines can be returned for the same address
	// 0. not seen multiple files returned but code is defensive just in case
	// 1. release build of {return atomicType++;} returns 3 lines for same address
	void AddFileLine(const std::wstring & fileName, unsigned int lineNumber) const
	{
		fileLines[fileName].emplace(lineNumber, false);
	}
};
