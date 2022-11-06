#pragma once

#include <utility>

namespace GLib::Util
{
	struct PairHash
	{
		template <typename T1, typename T2>
		size_t operator()(const std::pair<T1, T2> & pair) const
		{
			return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
		}
	};
}