
#include <GLib/NoCase.h>

#include <boost/test/unit_test.hpp>

#include <set>
#include <unordered_set>

#include "TestUtils.h"

template <typename CharType>
using CaseInsensitiveSet = std::set<std::basic_string<CharType>, GLib::NoCaseLess<CharType>>;

template <typename CharType>
using UnorderedCaseInsensitiveSet = std::unordered_set<std::basic_string<CharType>, GLib::NoCaseHash<CharType>, GLib::NoCaseEquality<CharType>>;

AUTO_TEST_SUITE(NoCaseTests)

AUTO_TEST_CASE(NoCaseLessChar)
{
	TEST(!GLib::NoCaseLess<char>()("a", "a"));
	TEST(!GLib::NoCaseLess<char>()("a", ""));
	TEST(GLib::NoCaseLess<char>()("", "a"));
	TEST(GLib::NoCaseLess<char>()("a", "b"));
	TEST(!GLib::NoCaseLess<char>()("b", "a"));

	CaseInsensitiveSet<char> set {"AbCdE", "aBcDe"};
	TEST(size_t {1} == set.size());
}

AUTO_TEST_CASE(NoCaseHashChar)
{
	TEST(GLib::NoCaseHash<char>()("AbCdE") == GLib::NoCaseHash<char>()("aBcDe"));
	TEST(GLib::NoCaseEquality<char>()("AbCdE", "aBcDe"));

	UnorderedCaseInsensitiveSet<char> set {"AbCdE", "aBcDe"};
	TEST(size_t {1} == set.size());
}

AUTO_TEST_SUITE_END()