#pragma once

#include "Fwd.h"

#include <chrono>
#include <string_view>

class Scope
{
	using TimePoint = std::chrono::high_resolution_clock::time_point;

	GLib::Flog::Level level;
	std::string_view prefix;
	std::string_view scopeText;
	std::string_view stem;
	TimePoint start;

public:
	Scope(GLib::Flog::Level level, std::string_view prefix, std::string_view scope, std::string_view stem)
		: level(level)
		, prefix(prefix)
		, scopeText(scope)
		, stem(stem)
		, start(std::chrono::high_resolution_clock::now())
	{}

	GLib::Flog::Level Level() const
	{
		return level;
	}

	std::string_view Prefix() const
	{
		return prefix;
	}

	std::string_view ScopeText() const
	{
		return scopeText;
	}

	std::string_view Stem() const
	{
		return stem;
	}

	auto Duration() const
	{
		return std::chrono::high_resolution_clock::now() - start;
	}

	template <typename T>
	auto Duration() const
	{
		return std::chrono::duration_cast<T>(Duration());
	}
};