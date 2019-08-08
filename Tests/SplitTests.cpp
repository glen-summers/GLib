#include <boost/test/unit_test.hpp>

#include <GLib/split.h>
#include "GLib/cvt.h"

#include "TestUtils.h"

BOOST_AUTO_TEST_SUITE(SplitTests)

BOOST_AUTO_TEST_CASE(TestString)
{
	GLib::Util::Splitter s("a<->bc<->def<->ghijkl", "<->");
	std::vector<std::string> expected { "a", "bc", "def", "ghijkl" };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(TestStringView)
{
	GLib::Util::Detail::Splitter<std::string_view> s("a<->bc<->def<->ghijkl", "<->");
	std::vector<std::string_view> expected { "a", "bc", "def", "ghijkl" };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(TestFor)
{
	std::vector<std::string> result;
	for (const auto & value : GLib::Util::Splitter("a<->bc<->def<->ghijkl", "<->"))
	{
		result.push_back(value);
	}

	std::vector<std::string> expected { "a", "bc", "def", "ghijkl" };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
}

BOOST_AUTO_TEST_CASE(TestEmptyString)
{
	GLib::Util::Splitter s("");
	std::vector<std::string> expected { };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(TestEmptyokens)
{
	GLib::Util::Splitter s(",,,");
	std::vector<std::string> expected { "","","","" };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(TestWhitespace1)
{
	GLib::Util::Splitter s("      ");
	std::vector<std::string> expected { "      " };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(TestWhitespace2)
{
	GLib::Util::Splitter s(" ,  ,  , ");
	std::vector<std::string> expected { " ","  ","  "," " };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(TestNoMatch)
{
	GLib::Util::Splitter s("Splitter");
	std::vector<std::string> expected { "Splitter" };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(TestFunc)
{
	std::vector<std::string> expected { "a","b","c","d" };
	std::vector<std::string> actual;
	std::string value = "a,b,c,d";
	GLib::Util::Split(value, std::back_inserter(actual));
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestViewFunc)
{
	std::vector<std::string_view> expected { "a","b","c","d" };
	std::vector<std::string_view> actual;
	std::string_view value = "a,b,c,d";
	GLib::Util::Split(value, std::back_inserter(actual));
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestWFunc)
{
	std::vector<std::string> expected { "a","b","c","d" };
	std::vector<std::wstring> actual;
	std::wstring value = L"a,b,c,d";
	GLib::Util::Split(value, std::back_inserter(actual));

	// workaround wstring generates boost compile errors
	std::vector<std::string> u8Actual;
	std::transform(actual.begin(), actual.end(), std::back_inserter(u8Actual),
		[](const std::wstring w) -> std::string { return GLib::Cvt::w2a(w); });

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), u8Actual.begin(), u8Actual.end());
}

BOOST_AUTO_TEST_CASE(EmptyDeliminatorIsError)
{
	std::string value = "a,b,c,d";
	std::vector<std::string> actual;

	GLIB_CHECK_LOGIC_EXCEPTION({ GLib::Util::Split(value, std::back_inserter(actual), {}); }, "Delimiter is empty");
}

BOOST_AUTO_TEST_SUITE_END()
