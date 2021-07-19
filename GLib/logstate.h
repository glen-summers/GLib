#pragma once

#include "scope.h"

#include <GLib/genericoutstream.h>
#include <GLib/vectorstreambuffer.h>

#include <stack>
#include <tuple>

class LogState
{
	static constexpr auto DefaultCapacity = 256;
	using StreamType = GLib::Util::GenericOutStream<char, GLib::Util::VectorStreamBuffer<char, DefaultCapacity>>;

	mutable std::stack<Scope> scopes;
	mutable int depth {};
	mutable bool pending {};
	mutable std::string_view threadName {};
	mutable StreamType stream;

public:
	std::ostream & Stream() const
	{
		return stream.Stream();
	}

	const Scope & Top() const
	{
		return scopes.top();
	}

	auto TopAndPop() const
	{
		Scope top = scopes.top();
		scopes.pop();
		if (!pending)
		{
			--depth;
		}
		return std::make_tuple(top, std::exchange(pending, false));
	}

	void Push(const Scope & scope) const
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

	std::string_view ThreadName() const
	{
		return threadName;
	}

	void ThreadName(std::string_view name) const
	{
		threadName = name;
	}

	void Put(char c) const
	{
		stream.Stream().put(c);
	}

	std::string_view Get() const
	{
		return stream.Buffer().Get();
	}

	void Reset() const
	{
		stream.Buffer().Reset();
	}

	void Commit() const
	{
		++depth;
		pending = false;
	}
};
