
#include "wstringfix.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ConverterTests)

	BOOST_AUTO_TEST_CASE(EuroSymbol)
	{
		std::string utf8Value = "\xE2\x82\xAC";
		std::wstring utf16Value = L"\u20AC";

		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(Hold(utf8Value) == Hold(GLib::Cvt::w2a(utf16Value)));
	}

	BOOST_AUTO_TEST_CASE(DeseretSmallLetterYee)
	{
		std::string utf8Value = "\xF0\x90\x90\xB7";
		std::wstring expectedWideValue = L"\U00010437";

		BOOST_TEST(Hold(expectedWideValue) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(Hold(utf8Value) == Hold(GLib::Cvt::w2a(expectedWideValue)));
	}

	BOOST_AUTO_TEST_CASE(CjkUnifiedIdeograph24B62)
	{
		std::string utf8Value = "\xF0\xA4\xAD\xA2";
		std::wstring utf16Value = L"\U00024B62";

		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(Hold(utf8Value) == Hold(GLib::Cvt::w2a(utf16Value)));
	}

	BOOST_AUTO_TEST_CASE(SharpS)
	{
		std::string utf8Value = "\xC3\x9F";
		std::wstring utf16Value = L"\u00DF";
		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(Hold(utf8Value) == Hold(GLib::Cvt::w2a(utf16Value)));
	}

	BOOST_AUTO_TEST_CASE(CompareSimpleCases)
	{
		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase("", "")));
		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase("abc", "abc")));
		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase("ABC", "abc")));
		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase("abc", "ABC")));
	}

	BOOST_AUTO_TEST_CASE(CompareSimpleCases2)
	{
		BOOST_TEST(-1 == static_cast<int>(GLib::Cvt::CompareNoCase("abc", "abcd")));
		BOOST_TEST(1 == static_cast<int>(GLib::Cvt::CompareNoCase("abcd", "abc")));
	}

	BOOST_AUTO_TEST_CASE(DiaeresisCompare)
	{
		const char * lowercaseUWithDiaeresis = "\xc3\xbc";
		const char * uppercaseUWithDiaeresis = "\xc3\x9c";

		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase(lowercaseUWithDiaeresis, uppercaseUWithDiaeresis)));
		BOOST_TEST(-1 == static_cast<int>(GLib::Cvt::CompareNoCase("u", lowercaseUWithDiaeresis)));
		BOOST_TEST(-1 == static_cast<int>(GLib::Cvt::CompareNoCase("U", uppercaseUWithDiaeresis)));
	}

	BOOST_AUTO_TEST_CASE(StartWithNoCasePartialDecode)
	{
		const char * s1 = "abcd" "\xc3\xbc" "defg";
		
		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase(s1, 4, s1, 4)));
		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase(s1, 5, s1, 5)));
		BOOST_TEST(0 == static_cast<int>(GLib::Cvt::CompareNoCase(s1, 6, s1, 6)));
	}

BOOST_AUTO_TEST_SUITE_END()