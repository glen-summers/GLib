#pragma once

#include "Scope.h"

#include <GLib/GenericOutStream.h>
#include <GLib/VectorStreamBuffer.h>

#include <stack>
#include <tuple>
#include <utility>

class LogState
{
	static constexpr auto defaultCapacity = 256;
	using StreamType = GLib::Util::GenericOutStream<char, GLib::Util::VectorStreamBuffer<char, defaultCapacity>>;

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

	Scope const & Top() const
	{
		return scopes.top();
	}

	auto TopAndPop() const
	{
		Scope const top = scopes.top();
		scopes.pop();
		if (!pending)
		{
			--depth;
		}
		return std::make_tuple(top, std::exchange(pending, false));
	}

	void Push(Scope const & scope) const
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

	void ThreadName(std::string_view const name) const
	{
		threadName = name;
	}

	void Put(char const c) const
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
