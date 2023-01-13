#pragma once

#include "Types.h"

class Address // NOLINT(bugprone-exception-escape)
{
	unsigned char const oldData;
	ULONG const symbolId;

	mutable FileLines fileLines; // use sparse container?
	mutable bool visited {};

public:
	Address(unsigned char oldData, ULONG symbolId);

	unsigned char OldData() const;

	FileLines const & FileLines() const;

	bool Visited() const;

	void Visit() const;

	ULONG SymbolId() const;

	void AddFileLine(std::wstring const & fileName, unsigned int lineNumber) const;
};
