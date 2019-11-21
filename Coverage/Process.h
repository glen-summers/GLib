#pragma once

#include "Function.h"

#include <map>

using Threads = std::map<unsigned int, void*>;

struct Process // ren ProcessInfo and encapsualate as class
{
	unsigned int id;
	Addresses addresses;
	Threads threads;
	std::map<unsigned long, Function> indexToFunction; // can be unordered

	Process(unsigned int id) : id{id}
	{}
};

using Processes = std::map<unsigned int, Process>;

