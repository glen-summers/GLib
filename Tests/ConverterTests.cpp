
#include "wstringfix.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ConverterTests)

	BOOST_AUTO_TEST_CASE(EuroSymbol)
	{
		std::string utf8Value = "\xE2\x82\xAC";
		std::wstring utf16Value = L"\x20AC";

		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(Hold(utf8Value) == Hold(GLib::Cvt::w2a(utf16Value)));
	}

	BOOST_AUTO_TEST_CASE(DeseretSmallLetterYee)
	{
		std::string utf8Value = "\xF0\x90\x90\xB7";
		std::wstring utf16Value = L"\xD801\xDC37";

		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(Hold(utf8Value) == Hold(GLib::Cvt::w2a(utf16Value)));
	}

	BOOST_AUTO_TEST_CASE(CjkUnifiedIdeograph24B62)
	{
		std::string utf8Value = "\xF0\xA4\xAD\xA2";
		std::wstring utf16Value = L"\xD852\xDF62";

		BOOST_TEST(Hold(utf16Value) == Hold(GLib::Cvt::a2w(utf8Value)));
		BOOST_TEST(Hold(utf8Value) == Hold(GLib::Cvt::w2a(utf16Value)));
	}

BOOST_AUTO_TEST_SUITE_END()