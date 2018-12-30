#pragma once

#include "Types.h"

class Address
{
	unsigned char oldData;
	FileLines fileLines; // use sparse container?
	bool visited;

public:
	Address(unsigned char oldData) : oldData(oldData), visited()
	{}

	unsigned char OldData() const { return oldData; }
	const FileLines & FileLines() const { return fileLines; }
	bool Visited() const { return visited; }

	void Visit()
	{
		visited = true;
	}

	void AddFileLine(const std::wstring & fileName, unsigned int lineNumber)
	{
		fileLines[fileName].insert({ lineNumber, false });
	}
};

