
#include <GLib/IcuUtils.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(IcuUtilsTests)

	BOOST_AUTO_TEST_CASE(CompareSimpleCases)
	{
		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("", "")));
		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abc", "abc")));
		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("ABC", "abc")));
		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abc", "ABC")));
		BOOST_TEST(-1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abc", "abcd")));
		BOOST_TEST(1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("abcd", "abc")));
	}

	BOOST_AUTO_TEST_CASE(CompareSimpleCases2)
	{
		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase("\xC4\xB0", "i", "tr")));
	}

	BOOST_AUTO_TEST_CASE(DiaeresisCompare)
	{
		const char * lowercaseUWithDiaeresis = "\xc3\xbc";
		const char * uppercaseUWithDiaeresis = "\xc3\x9c";

		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(lowercaseUWithDiaeresis, uppercaseUWithDiaeresis)));
		BOOST_TEST(-1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("u", lowercaseUWithDiaeresis)));
		BOOST_TEST(-1 == static_cast<int>(GLib::IcuUtils::CompareNoCase("U", uppercaseUWithDiaeresis)));
	}

	BOOST_AUTO_TEST_CASE(StartWithNoCasePartialDecode)
	{
		const char * s1 = "abcd" "\xc3\xbc" "defg";

		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(s1, 4, s1, 4)));
		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(s1, 5, s1, 5)));
		BOOST_TEST(0 == static_cast<int>(GLib::IcuUtils::CompareNoCase(s1, 6, s1, 6)));
	}

	BOOST_AUTO_TEST_CASE(LowerUpper)
	{
		BOOST_TEST("abcde" == GLib::IcuUtils::ToLower("AbCdE"));
		BOOST_TEST("ABCDE" == GLib::IcuUtils::ToUpper("AbCdE"));

		BOOST_TEST("GROSS" == GLib::IcuUtils::ToUpper("gro\xC3\x9F"));
		BOOST_TEST("gross" == GLib::IcuUtils::ToLower("GROSS"));

		BOOST_TEST("perch\xC3\xA9" == GLib::IcuUtils::ToLower("PERCH\xC3\x89"));
		BOOST_TEST("PERCH\xC3\x89" == GLib::IcuUtils::ToUpper("perch\xC3\xA9"));
	}

	BOOST_AUTO_TEST_CASE(TurkishI)
	{
		BOOST_TEST("I" == GLib::IcuUtils::ToUpper("i"));
		BOOST_TEST("\xC4\xB0" == GLib::IcuUtils::ToUpper("i", "tr"));
	}

BOOST_AUTO_TEST_SUITE_END()