#pragma once

#include "Types.h"

class Address
{
	unsigned char const oldData;
	ULONG const symbolId;

	mutable FileLines fileLines; // use sparse container?
	mutable bool visited {};

public:
	explicit Address(unsigned char const oldData, ULONG const symbolId)
		: oldData(oldData)
		, symbolId(symbolId)
	{}

	unsigned char OldData() const
	{
		return oldData;
	}

	FileLines const & FileLines() const
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

	ULONG SymbolId() const
	{
		return symbolId;
	}

	// multiple lines can be returned for the same address
	// 0. not seen multiple files returned but code is defensive just in case
	// 1. release build of {return atomicType++;} returns 3 lines for same address
	void AddFileLine(std::wstring const & fileName, unsigned int const lineNumber) const
	{
		fileLines[fileName].emplace(lineNumber, false);
	}
};
