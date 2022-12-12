#pragma once

#include "Types.h"

class Address
{
	unsigned char const oldData;
	ULONG const symbolId;
	mutable bool visited {};

public:
	explicit Address(unsigned char const oldData, ULONG const symbolId)
		: oldData(oldData)
		, symbolId(symbolId)
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

	unsigned char OldData() const
	{
		return oldData;
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
	static void AddFileLine(std::wstring const & fileName, unsigned int const lineNumber)
	{
		GetFileLines()[fileName].emplace(lineNumber, false);
	}
};
