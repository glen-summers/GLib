
#include <boost/test/unit_test.hpp>

#include "TestUtils.h"

AUTO_TEST_SUITE(ConverterTests)

AUTO_TEST_CASE(EuroSymbol)
{
	std::string utf8Value = "\xE2\x82\xAC";
	std::wstring utf16Value = L"\u20AC";

	TEST(utf16Value == GLib::Cvt::A2W(utf8Value));
	TEST(utf8Value == GLib::Cvt::W2A(utf16Value));
}

AUTO_TEST_CASE(DeseretSmallLetterYee)
{
	std::string utf8Value = "\xF0\x90\x90\xB7";
	std::wstring expectedWideValue = L"\U00010437";

	TEST(expectedWideValue == GLib::Cvt::A2W(utf8Value));
	TEST(utf8Value == GLib::Cvt::W2A(expectedWideValue));
}

AUTO_TEST_CASE(CjkUnifiedIdeograph24B62)
{
	std::string utf8Value = "\xF0\xA4\xAD\xA2";
	std::wstring utf16Value = L"\U00024B62";

	TEST(utf16Value == GLib::Cvt::A2W(utf8Value));
	TEST(utf8Value == GLib::Cvt::W2A(utf16Value));
}

AUTO_TEST_CASE(SharpS)
{
	std::string utf8Value = "\xC3\x9F";
	std::wstring utf16Value = L"\u00DF";
	TEST(utf16Value == GLib::Cvt::A2W(utf8Value));
	TEST(utf8Value == GLib::Cvt::W2A(utf16Value));
}

AUTO_TEST_SUITE_END()