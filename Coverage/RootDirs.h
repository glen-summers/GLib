#pragma once

#include <filesystem>
#include <set>

namespace Detail
{
	inline bool CommonRoot(std::filesystem::path const & p1, std::filesystem::path const & p2, std::filesystem::path & root)
	{
		// assert, are absolute etc.
		bool result = false;
		for (auto it1 = p1.begin(), it2 = p2.begin(); it1 != p1.end() && it2 != p2.end(); ++it1, ++it2)
		{
			if (*it1 != *it2)
			{
				break;
			}
			root /= *it1;
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

inline std::tuple<std::filesystem::path, std::filesystem::path> Reduce(std::filesystem::path const & path,
																																			 std::set<std::filesystem::path> const & paths)
{
	for (auto const & p : paths)
	{
		// avoid as calls GetFinalPathNameByHandleW?
		auto rel = relative(path, p);
		if (!rel.empty())
		{
			return {p, rel};
		}
	}
	throw std::runtime_error("Path not reduced");
}