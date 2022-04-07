#pragma once

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

		ScopedFunction(const ScopedFunction &) = delete;
		ScopedFunction & operator=(const ScopedFunction &) = delete;
		ScopedFunction(ScopedFunction &&) noexcept = default;
		ScopedFunction & operator=(ScopedFunction &&) noexcept = default;

		~ScopedFunction()
		{
			function();
		}
	};

	template <typename Function>
	auto Scope(Function && exit)
	{
		return Detail::ScopedFunction<Function>(std::forward<Function>(exit));
	}
}