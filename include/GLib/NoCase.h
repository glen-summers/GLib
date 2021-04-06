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
		bool operator()(const std::string & s1, const std::string & s2) const
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
		size_t operator()(const std::string & key) const
		{
			size_t val {};
			for (char c : IcuUtils::ToLower(key))
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
		bool operator()(const std::string & left, const std::string & right) const
		{
			return IcuUtils::CompareNoCase(left, right) == IcuUtils::CompareResult::Equal;
		}
	};
}
