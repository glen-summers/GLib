#include <boost/test/unit_test.hpp>

#include "GLib/compat.h"

#include "TestUtils.h"

struct UnmangleStruct
{};

class UnmangleClass
{};

BOOST_AUTO_TEST_SUITE(CompatTests)

BOOST_AUTO_TEST_CASE(LocalTime)
{
	tm tm {};
	GLib::Compat::LocalTime(tm, 0);

	BOOST_TEST(tm.tm_sec == 0);
	BOOST_TEST(tm.tm_min == 0);
	BOOST_TEST(tm.tm_hour == 0);
	BOOST_TEST(tm.tm_mday == 1);
	BOOST_TEST(tm.tm_mon == 0);
	BOOST_TEST(tm.tm_year == 70);
	BOOST_TEST(tm.tm_wday == 4);
	BOOST_TEST(tm.tm_yday == 0);
	BOOST_TEST(tm.tm_isdst == 0);
}

BOOST_AUTO_TEST_CASE(GmTime)
{
	tm tm {};
	GLib::Compat::GmTime(tm, 0);

	BOOST_TEST(tm.tm_sec == 0);
	BOOST_TEST(tm.tm_min == 0);
	BOOST_TEST(tm.tm_hour == 0);
	BOOST_TEST(tm.tm_mday == 1);
	BOOST_TEST(tm.tm_mon == 0);
	BOOST_TEST(tm.tm_year == 70);
	BOOST_TEST(tm.tm_wday == 4);
	BOOST_TEST(tm.tm_yday == 0);
	BOOST_TEST(tm.tm_isdst == 0);
}

BOOST_AUTO_TEST_CASE(StrError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION(
	{
		errno = ENOENT;
		GLib::Compat::StrError("error");
	}, "error : No such file or directory");
}

BOOST_AUTO_TEST_CASE(Unmangle)
{
	BOOST_TEST("UnmangleStruct" == GLib::Compat::Unmangle(typeid(UnmangleStruct).name()));
	BOOST_TEST("UnmangleClass" == GLib::Compat::Unmangle(typeid(UnmangleClass).name()));
}

BOOST_AUTO_TEST_CASE(ProcessName)
{
	auto name = GLib::Compat::ProcessName();
	auto exe = name.rfind(".exe");
	if (exe == name.size() - 4) name.erase(exe);
	BOOST_TEST("Tests" == name);
}

BOOST_AUTO_TEST_SUITE_END()