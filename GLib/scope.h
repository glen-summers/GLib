#pragma once

#include "fwd.h"

#include <chrono>

struct Scope
{
	GLib::Flog::Level level;
	const char * scope;
	const char * stem;
	std::chrono::high_resolution_clock::time_point start;
};
