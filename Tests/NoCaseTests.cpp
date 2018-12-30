#include <boost/test/unit_test.hpp>

#include "GLib/NoCase.h"

#include <set>
#include <unordered_set>

template <typename CharType> using CaseInsensitiveSet = std::set<std::basic_string<CharType>, GLib::NoCaseLess<CharType>>;

template <typename CharType> using UnorderedCaseInsensitiveSet = std::unordered_set<std::basic_string<CharType>,
	GLib::NoCaseHash<CharType>, GLib::NoCaseEquality<CharType>>;

BOOST_AUTO_TEST_SUITE(NoCaseTests)

BOOST_AUTO_TEST_CASE(NoCaseLessChar)
{
	BOOST_TEST(!GLib::NoCaseLess<char>()("a", "a"));
	BOOST_TEST(!GLib::NoCaseLess<char>()("a", ""));
	BOOST_TEST(GLib::NoCaseLess<char>()("", "a"));
	BOOST_TEST(GLib::NoCaseLess<char>()("a", "b"));
	BOOST_TEST(!GLib::NoCaseLess<char>()("b", "a"));

	CaseInsensitiveSet<char> set{ "AbCdE", "aBcDe" };
	BOOST_TEST(size_t{1} == set.size());
}

BOOST_AUTO_TEST_CASE(NoCaseHashChar)
{
	BOOST_TEST(GLib::NoCaseHash<char>()("AbCdE") == GLib::NoCaseHash<char>()("aBcDe"));
	BOOST_TEST(GLib::NoCaseEquality<char>()("AbCdE", "aBcDe"));

	UnorderedCaseInsensitiveSet<char> set{ "AbCdE", "aBcDe" };
	BOOST_TEST(size_t{1} == set.size());
}

BOOST_AUTO_TEST_SUITE_END()