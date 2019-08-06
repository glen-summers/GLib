#pragma once

#include <filesystem>
#include <set>

namespace Detail
{
	inline bool CommonRoot(const std::filesystem::path & p1, const std::filesystem::path & p2, std::filesystem::path & root)
	{
		// assert, are absolute etc.
		bool result = false;
		for (auto it1 = p1.begin(), it2 = p2.begin(); it1 != p1.end() && it2 != p2.end(); ++it1, ++it2)
		{
			auto part1 = *it1;
			auto part2 = *it2;
			if (part1 != part2)
			{
				break;
			}
			root /= part1;
			result = true;
		}

		return result;
	}
}

inline void RootDirectories(std::set<std::filesystem::path> & paths)
{
	for (auto it = paths.begin(); it != paths.end();)
	{
		auto current = it++;
		auto next = it;
		if (next == paths.end())
		{
			break;
		}
		std::filesystem::path root;
		if (Detail::CommonRoot(*current, *next, root))
		{
			paths.erase(current);
			paths.erase(next);
			it = paths.insert(root).first;
		}
	}
}

inline std::filesystem::path Reduce(const std::filesystem::path & path, const std::set<std::filesystem::path> & paths)
{
	for (const auto & p : paths)
	{
		// avoid as calls GetFinalPathNameByHandleW?
		auto rel = relative(path, p);
		if (!rel.empty())
		{
			return rel;
		}
	}
	throw std::runtime_error("Path not reduced");
}