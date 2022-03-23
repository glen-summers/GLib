#pragma once

#include <stdexcept>
#include <utility>

namespace GLib::Util
{
	template <typename Target, typename Source>
	Target CheckedCast(Source source)
	{
		constexpr auto max = std::numeric_limits<Target>::max();
		constexpr auto min = std::numeric_limits<Target>::min();

		if (std::cmp_greater(source, max))
		{
			throw std::runtime_error("Overflow");
		}

		if (std::cmp_less(source, min))
		{
			throw std::runtime_error("Underflow");
		}

		return static_cast<Target>(source);
	};
}