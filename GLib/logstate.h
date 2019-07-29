#pragma once

#include "GLib/genericoutstream.h"
#include "GLib/vectorstreambuffer.h"
#include "scope.h"

#include <stack>

// class\accesors
struct LogState
{
	using Stream = GLib::Util::GenericOutStream<char, GLib::Util::VectorStreamBuffer<char>>;

	std::stack<Scope> scopes;
	int depth {};
	bool pending {};
	const char * threadName {};
	Stream stream { std::ios_base::boolalpha };
};

