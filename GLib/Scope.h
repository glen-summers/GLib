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
	Scope(GLib::Flog::Level const level, std::string_view const prefix, std::string_view const scope, std::string_view const stem)
		: level(level)
		, prefix(prefix)
		, scopeText(scope)
		, stem(stem)
		, start(std::chrono::high_resolution_clock::now())
	{}

	[[nodiscard]] GLib::Flog::Level Level() const
	{
		return level;
	}

	[[nodiscard]] std::string_view Prefix() const
	{
		return prefix;
	}

	[[nodiscard]] std::string_view ScopeText() const
	{
		return scopeText;
	}

	[[nodiscard]] std::string_view Stem() const
	{
		return stem;
	}

	[[nodiscard]] auto Duration() const
	{
		return std::chrono::high_resolution_clock::now() - start;
	}

	template <typename T>
	[[nodiscard]] auto Duration() const
	{
		return std::chrono::duration_cast<T>(Duration());
	}
};