#include <boost/test/unit_test.hpp>

#include <GLib/split.h>


BOOST_AUTO_TEST_SUITE(SplitTests)

BOOST_AUTO_TEST_CASE(Test1)
{
	GLib::Util::Splitter s("a<->bc<->def<->ghijkl", "<->");
	std::vector<std::string> expected { "a", "bc", "def", "ghijkl" };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
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

BOOST_AUTO_TEST_SUITE_END()
