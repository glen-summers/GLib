#include "pch.h"

#include "Address.h"

Address::Address(unsigned char const oldData, ULONG const symbolId)
	: oldData(oldData)
	, symbolId(symbolId)
{}

unsigned char Address::OldData() const
{
	return oldData;
}

FileLines const & Address::FileLines() const
{
	return fileLines;
}

bool Address::Visited() const
{
	return visited;
}

void Address::Visit() const
{
	visited = true;
}

ULONG Address::SymbolId() const
{
	return symbolId;
}

// multiple lines can be returned for the same address
// 0. not seen multiple files returned but code is defensive just in case
// 1. release build of {return atomicType++;} returns 3 lines for same address
void Address::AddFileLine(std::wstring const & fileName, unsigned int const lineNumber) const
{
	fileLines[fileName].emplace(lineNumber, false);
}
