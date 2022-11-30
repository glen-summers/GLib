
#include <GLib/IcuUtils.h>

#include <boost/test/unit_test.hpp>

#include "TestUtils.h"

AUTO_TEST_SUITE(IcuUtilsTests)

AUTO_TEST_CASE(CompareSimpleCases)
{
	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("", "")));
	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abc", "abc")));
	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("ABC", "abc")));
	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abc", "ABC")));
	TEST(-1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abc", "abcd")));
	TEST(1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abcd", "abc")));
}

AUTO_TEST_CASE(CompareSimpleCases2)
{
	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("\xC4\xB0", "i", "tr")));
}

AUTO_TEST_CASE(DiaeresisCompare)
{
	auto const * lowercaseUWithDiaeresis = "\xc3\xbc";
	auto const * uppercaseUWithDiaeresis = "\xc3\x9c";

	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(lowercaseUWithDiaeresis, uppercaseUWithDiaeresis)));
	TEST(-1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("u", lowercaseUWithDiaeresis)));
	TEST(-1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("U", uppercaseUWithDiaeresis)));
}

AUTO_TEST_CASE(StartWithNoCasePartialDecode)
{
	auto const * s = "abcd"
									 "\xc3\xbc"
									 "defg";

	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(s, s, 4)));
	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(s, s, 5)));
	TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(s, s, 6)));
}

AUTO_TEST_CASE(LowerUpper)
{
	TEST("abcde" == GLib::IcuUtils::ToLower("AbCdE"));
	TEST("ABCDE" == GLib::IcuUtils::ToUpper("AbCdE"));

	TEST("GROSS" == GLib::IcuUtils::ToUpper("gro\xC3\x9F"));
	TEST("gross" == GLib::IcuUtils::ToLower("GROSS"));

	TEST("perch\xC3\xA9" == GLib::IcuUtils::ToLower("PERCH\xC3\x89"));
	TEST("PERCH\xC3\x89" == GLib::IcuUtils::ToUpper("perch\xC3\xA9"));
}

AUTO_TEST_CASE(TurkishI)
{
	TEST("I" == GLib::IcuUtils::ToUpper("i"));
	TEST("\xC4\xB0" == GLib::IcuUtils::ToUpper("i", "tr"));
}

AUTO_TEST_SUITE_END()