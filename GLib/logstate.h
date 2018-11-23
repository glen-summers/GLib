#pragma once

#include "GLib/genericoutstream.h"
#include "GLib/vectorstreambuffer.h"
#include "scope.h"

#include <stack>

struct LogState
{
	using Stream = GLib::Util::GenericOutStream<char, GLib::Util::VectorStreamBuffer<char>>;

	std::stack<Scope> scopes;
	const char * pendingScope;
	int depth;
	const char * threadName;
	Stream stream;

	LogState()
		: pendingScope()
		, depth()
		, threadName()
		, stream { std::ios_base::boolalpha }
	{}
};

