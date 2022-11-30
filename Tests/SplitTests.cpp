#include <GLib/Split.h>

#include <GLib/ConsecutiveFind.h>
#include <GLib/Cvt.h>

#include <boost/test/unit_test.hpp>

#include "TestUtils.h"

AUTO_TEST_SUITE(SplitTests)

AUTO_TEST_CASE(TestString)
{
	GLib::Util::Splitter const s("a<->bc<->def<->ghijkl", "<->");
	std::vector<std::string> expected {"a", "bc", "def", "ghijkl"};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestStringView)
{
	GLib::Util::SplitterView const s("a<->bc<->def<->ghijkl", "<->");
	std::vector<std::string_view> expected {"a", "bc", "def", "ghijkl"};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestFor)
{
	std::vector<std::string> result;
	for (auto const & value : GLib::Util::Splitter("a<->bc<->def<->ghijkl", "<->"))
	{
		result.push_back(value);
	}

	std::vector<std::string> expected {"a", "bc", "def", "ghijkl"};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
}

AUTO_TEST_CASE(TestEmptyString)
{
	GLib::Util::Splitter const s("");
	std::vector<std::string> expected {""};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestEmptyoken)
{
	GLib::Util::Splitter const s(",");
	std::vector<std::string> expected {"", ""};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestEmptyokens)
{
	GLib::Util::Splitter const s(",,,");
	std::vector<std::string> expected {"", "", "", ""};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestWhitespace1)
{
	GLib::Util::Splitter const s("      ");
	std::vector<std::string> expected {"      "};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestWhitespace2)
{
	GLib::Util::Splitter const s(" ,  ,  , ");
	std::vector<std::string> const expected {" ", "  ", "  ", " "};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestNoMatch)
{
	GLib::Util::Splitter const s("Splitter");
	std::vector<std::string> const expected {"Splitter"};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());
}

AUTO_TEST_CASE(TestFunc)
{
	std::vector<std::string> const expected {"a", "b", "c", "d"};
	std::vector<std::string> actual;
	std::string const value = "a,b,c,d";
	GLib::Util::Split(value, std::back_inserter(actual));
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

AUTO_TEST_CASE(TestViewFunc)
{
	std::vector<std::string_view> const expected {"a", "b", "c", "d"};
	std::vector<std::string_view> actual;
	std::string_view constexpr value = "a,b,c,d";
	GLib::Util::Split(value, std::back_inserter(actual));
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

AUTO_TEST_CASE(TestWFunc)
{
	std::vector<std::string> const expected {"a", "b", "c", "d"};
	std::vector<std::wstring> actual;
	std::wstring const value = L"a,b,c,d";
	GLib::Util::Split(value, std::back_inserter(actual));

	// workaround wstring generates boost compile errors
	std::vector<std::string> u8Actual;
	std::transform(actual.begin(), actual.end(), std::back_inserter(u8Actual), [](std::wstring const & w) -> std::string { return GLib::Cvt::W2A(w); });

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), u8Actual.begin(), u8Actual.end());
}

AUTO_TEST_CASE(EmptyDeliminatorIsError)
{
	std::string const value = "a,b,c,d";
	std::vector<std::string> actual;

	GLIB_CHECK_LOGIC_EXCEPTION({ GLib::Util::Split(value, std::back_inserter(actual), {}); }, "Delimiter is empty");
}

AUTO_TEST_CASE(ConsecutiveFindTest)
{
	using Pair = std::pair<int, size_t>;
	std::vector const values {1, 1, 2, 3, 3, 3};
	std::vector<Pair> const expected = {{1, 2}, {2, 1}, {3, 3}};

	std::vector<Pair> result;
	std::vector<int>::const_iterator next;

	for (auto it = values.cbegin(), end = values.cend(); it != end; it = next)
	{
		next = GLib::Util::ConsecutiveFind(it, end);
		result.emplace_back(*it, std::distance(it, next));
	}

	CHECK_EQUAL_COLLECTIONS(expected.cbegin(), expected.cend(), result.cbegin(), result.cend());
}

AUTO_TEST_CASE(ConsecutiveFindPred)
{
	using Pair = std::pair<int, std::string>;
	auto pred = [](Pair const & p1, Pair const & p2) { return p1.second != p2.second; };

	std::vector<Pair> const values {{1, "1"}, {2, "1"}, {3, "2"}, {4, "3"}, {5, "3"}, {6, "3"}};
	std::vector<std::pair<std::string, size_t>> const expected {{"1", 2}, {"2", 1}, {"3", 3}};

	std::vector<std::pair<std::string, size_t>> result;
	std::vector<Pair>::const_iterator next;

	for (auto it = values.cbegin(), end = values.cend(); it != end; it = next)
	{
		next = GLib::Util::ConsecutiveFind(it, end, pred);
		result.emplace_back(it->second, std::distance(it, next));
	}

	CHECK_EQUAL_COLLECTIONS(expected.cbegin(), expected.cend(), result.cbegin(), result.cend());
}

AUTO_TEST_SUITE_END()
