
#include "wstringfix.h"

#include <boost/test/unit_test.hpp>

#include <set>
#include <unordered_set>

template <typename CharType> using CaseInsensitiveSet = std::set<std::basic_string<CharType>,
	GLib::Cvt::NoCaseLess<CharType>>;

template <typename CharType> using UnorderedCaseInsensitiveSet = std::unordered_set<std::basic_string<CharType>,
	GLib::Cvt::NoCaseHash<CharType>, GLib::Cvt::NoCaseEquality<CharType>>;

BOOST_AUTO_TEST_SUITE(ConverterTests)

	BOOST_AUTO_TEST_CASE(EuroSymbol)
	{
		std::string utf8Value = "\xE2\x82\xAC";
		std::wstring utf16Value = L"\u20AC";

		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(utf8Value == GLib::Cvt::w2a(utf16Value));
	}

	BOOST_AUTO_TEST_CASE(DeseretSmallLetterYee)
	{
		std::string utf8Value = "\xF0\x90\x90\xB7";
		std::wstring expectedWideValue = L"\U00010437";

		BOOST_TEST(Hold(expectedWideValue) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(utf8Value == GLib::Cvt::w2a(expectedWideValue));
	}

	BOOST_AUTO_TEST_CASE(CjkUnifiedIdeograph24B62)
	{
		std::string utf8Value = "\xF0\xA4\xAD\xA2";
		std::wstring utf16Value = L"\U00024B62";

		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(utf8Value == GLib::Cvt::w2a(utf16Value));
	}

	BOOST_AUTO_TEST_CASE(SharpS)
	{
		std::string utf8Value = "\xC3\x9F";
		std::wstring utf16Value = L"\u00DF";
		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(utf8Value == GLib::Cvt::w2a(utf16Value));
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

	BOOST_AUTO_TEST_CASE(NoCaseLessChar)
	{
		BOOST_TEST(!GLib::Cvt::NoCaseLess<char>()("a", "a"));
		BOOST_TEST(!GLib::Cvt::NoCaseLess<char>()("a", ""));
		BOOST_TEST(GLib::Cvt::NoCaseLess<char>()("", "a"));
		BOOST_TEST(GLib::Cvt::NoCaseLess<char>()("a", "b"));
		BOOST_TEST(!GLib::Cvt::NoCaseLess<char>()("b", "a"));

		CaseInsensitiveSet<char> set{ "AbCdE", "aBcDe" };
		BOOST_TEST(1 == set.size());
	}

	BOOST_AUTO_TEST_CASE(NoCaseLessWChar)
	{
		BOOST_TEST(!GLib::Cvt::NoCaseLess<wchar_t>()(L"a", L"a"));
		BOOST_TEST(!GLib::Cvt::NoCaseLess<wchar_t>()(L"a", L""));
		BOOST_TEST(GLib::Cvt::NoCaseLess<wchar_t>()(L"", L"a"));
		BOOST_TEST(GLib::Cvt::NoCaseLess<wchar_t>()(L"a", L"b"));
		BOOST_TEST(!GLib::Cvt::NoCaseLess<wchar_t>()(L"b", L"a"));

		CaseInsensitiveSet<wchar_t> set{ L"AbCdE", L"aBcDe" };
		BOOST_TEST(1 == set.size());
	}

	BOOST_AUTO_TEST_CASE(NoCaseHashChar)
	{
		BOOST_TEST(GLib::Cvt::NoCaseHash<char>()("AbCdE") == GLib::Cvt::NoCaseHash<char>()("aBcDe"));
		BOOST_TEST(GLib::Cvt::NoCaseEquality<char>()("AbCdE", "aBcDe"));

		UnorderedCaseInsensitiveSet<char> set{ "AbCdE", "aBcDe" };
		BOOST_TEST(1 == set.size());
	}

	BOOST_AUTO_TEST_CASE(LowerUpper)
	{
		BOOST_TEST("abcde" == GLib::Cvt::ToLower("AbCdE"));
		BOOST_TEST("ABCDE" == GLib::Cvt::ToUpper("AbCdE"));

		BOOST_TEST("GROSS" == GLib::Cvt::ToUpper("gro\xC3\x9F"));
		BOOST_TEST("gross" == GLib::Cvt::ToLower("GROSS", "de"));

		BOOST_TEST("perch\xC3\xA9" == GLib::Cvt::ToLower("PERCH\xC3\x89"));
		BOOST_TEST("PERCH\xC3\x89" == GLib::Cvt::ToUpper("perch\xC3\xA9"));
	}

	BOOST_AUTO_TEST_CASE(TurkishI)
	{
		BOOST_TEST("I" == GLib::Cvt::ToUpper("i"));
		BOOST_TEST("\xC4\xB0" == GLib::Cvt::ToUpper("i", "tr"));
	}

BOOST_AUTO_TEST_SUITE_END()