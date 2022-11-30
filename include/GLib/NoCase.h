#pragma once

#include <string>

#include <GLib/IcuUtils.h>

namespace GLib
{
	template <typename CharType>
	struct NoCaseLess;

	template <>
	struct NoCaseLess<char>
	{
		bool operator()(std::string_view const s1, std::string_view const s2) const
		{
			return IcuUtils::CompareNoCase(s1, s2) == IcuUtils::CompareResult::Less;
		}
	};

	template <typename CharType>
	class NoCaseHash;

	template <>
	class NoCaseHash<char>
	{
		static constexpr size_t fnvPrime = 16777619U;

	public:
		size_t operator()(std::string const & key) const
		{
			size_t val {};
			for (char const c : IcuUtils::ToLower(key))
			{
				val ^= static_cast<size_t>(c);
				val *= fnvPrime;
			}
			return val;
		}
	};

	template <typename CharType>
	struct NoCaseEquality;

	template <>
	struct NoCaseEquality<char>
	{
		bool operator()(std::string const & left, std::string const & right) const
		{
			return IcuUtils::CompareNoCase(left, right) == IcuUtils::CompareResult::Equal;
		}
	};
}
