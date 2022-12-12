#pragma once

#include <cstdlib>
#include <exception>
#include <utility>

namespace GLib::Detail
{
	template <typename Function>
	class ScopedFunction
	{
		Function function;

	public:
		explicit ScopedFunction(Function function) noexcept
			: function(std::move(function))
		{}

		ScopedFunction(ScopedFunction const &) = delete;
		ScopedFunction & operator=(ScopedFunction const &) = delete;
		ScopedFunction(ScopedFunction &&) noexcept = default;
		ScopedFunction & operator=(ScopedFunction &&) noexcept = default;

		~ScopedFunction()
		{
			function();
		}
	};

	template <typename Function>
	auto Scope(Function && exit) noexcept
	{
		return Detail::ScopedFunction<Function>(std::forward<Function>(exit));
	}
}