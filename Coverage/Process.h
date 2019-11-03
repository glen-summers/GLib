#pragma once

#include "Function.h"

#include <map>

using Threads = std::map<unsigned int, void*>;

struct Process
{
	unsigned int id;
	Addresses addresses;
	Threads threads;
	std::map<unsigned long, Function> indexToFunction;

	Process(unsigned int id) : id{id}
	{}
};

using Processes = std::map<unsigned int, Process>;

