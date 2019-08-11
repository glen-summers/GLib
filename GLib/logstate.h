#pragma once

#include "GLib/genericoutstream.h"
#include "GLib/vectorstreambuffer.h"
#include "scope.h"

#include <stack>

class LogState
{
	static constexpr auto DefaultCapacity = 256;
	using StreamType = GLib::Util::GenericOutStream<char, GLib::Util::VectorStreamBuffer<char, DefaultCapacity>>;

	std::stack<Scope> scopes;
	int depth;
	bool pending;
	const char * threadName;
	StreamType stream;

public:
	LogState() noexcept
		: depth{}, pending{}, threadName{}
	{}

	std::ostream & Stream()
	{
		return stream.Stream();
	}

	const Scope & Top() const
	{
		return scopes.top();
	}

	bool Pop()
	{
		scopes.pop();
		if (!pending)
		{
			--depth;
		}
		return std::exchange(pending, false);
	}

	void Push(const Scope & scope)
	{
		scopes.push(scope);
		pending = true;
	}

	int Depth() const
	{
		return depth;
	}

	bool Pending() const
	{
		return pending;
	}

	const char * ThreadName() const
	{
		return threadName;
	}

	void ThreadName(const char * name)
	{
		threadName= name;
	}

	void Put(char c)
	{
		stream.Stream().put(c);
	}

	std::string_view Get()
	{
		return stream.Buffer().Get();
	}

	void Reset()
	{
		stream.Buffer().Reset();
	}

	void Commit()
	{
		++depth;
		pending = false;
	}
};

