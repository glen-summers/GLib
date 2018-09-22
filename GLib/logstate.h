#pragma once

#include "genericoutstream.h"
#include "buffer.h"
#include "scope.h"

#include <stack>

struct LogState
{
	using Stream = GenericOutStream<char, Buffer>;

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

