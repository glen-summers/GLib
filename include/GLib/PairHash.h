#pragma once

#include <utility>

namespace GLib::Util
{
	struct PairHash
	{
		template <typename T1, typename T2>
		size_t operator()(std::pair<T1, T2> const & pair) const
		{
			return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
		}
	};
}