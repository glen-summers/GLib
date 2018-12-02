#pragma once

#include <chrono>
#include <utility>

class Scope
{
	typedef std::chrono::high_resolution_clock::time_point TimePoint;

	GLib::Flog::Level level;
	std::string prefix;
	std::string scopeText;
	std::string stem;
	TimePoint start;

public:
	Scope(GLib::Flog::Level level, std::string prefix, std::string scope, std::string stem)
		: level(level)
		, prefix(std::move(prefix))
		, scopeText(std::move(scope))
		, stem(std::move(stem))
		, start(std::chrono::high_resolution_clock::now())
	{}

	GLib::Flog::Level Level() const
	{
		return level;
	}

	const std::string  & Prefix() const
	{
		return prefix;
	}

	const std::string & ScopeText() const
	{
		return scopeText;
	}

	const std::string  & Stem() const
	{
		return stem;
	}

	auto Duration() const
	{
		return std::chrono::high_resolution_clock::now() - start;
	}
};