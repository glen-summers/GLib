#include <boost/test/unit_test.hpp>

#include "GLib/compat.h"
#include "GLib/scope.h"

#include "TestUtils.h"

struct UnmangleStruct
{};

class UnmangleClass
{};

BOOST_AUTO_TEST_SUITE(CompatTests)

BOOST_AUTO_TEST_CASE(LocalTimeZero)
{
	auto tz = GLib::Compat::GetEnv("TZ");
	GLib::Compat::SetEnv("TZ", "GMST0GMDT-1,M3.3.0,M10.1.0");
	GLib::Compat::TzSet();
	SCOPE(_, [&]() noexcept
	{
		if (tz) GLib::Compat::SetEnv("TZ", tz->c_str());
		else GLib::Compat::UnsetEnv("TZ");
		GLib::Compat::TzSet();
	});

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

BOOST_AUTO_TEST_CASE(LocalTimeSpecific)
{
	auto tz = GLib::Compat::GetEnv("TZ");
	GLib::Compat::SetEnv("TZ", "EST5EDT");
	GLib::Compat::TzSet();
	SCOPE(_, [&]() noexcept
	{
		if (tz) GLib::Compat::SetEnv("TZ", tz->c_str());
		else GLib::Compat::UnsetEnv("TZ");
		GLib::Compat::TzSet();
	});

	tm tm {};
	GLib::Compat::LocalTime(tm, 1569056285);

	BOOST_TEST(tm.tm_sec == 5);
	BOOST_TEST(tm.tm_min == 58);
	BOOST_TEST(tm.tm_hour == 4);
	BOOST_TEST(tm.tm_mday == 21);
	BOOST_TEST(tm.tm_mon == 8);
	BOOST_TEST(tm.tm_year == 119);
	BOOST_TEST(tm.tm_wday == 6);
	BOOST_TEST(tm.tm_yday == 263);
	BOOST_TEST(tm.tm_isdst == 1);
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
		GLib::Compat::AssertTrue(false, "error", ENOENT);
	}, "error : No such file or directory");

	GLIB_CHECK_RUNTIME_EXCEPTION(
	{
		GLib::Compat::AssertTrue(false, "error", ESRCH);
	}, "error : No such process");

	GLib::Compat::AssertTrue(true, "no error", 0);
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

BOOST_AUTO_TEST_CASE(Env)
{
	auto testVar = "TestVar";
	BOOST_CHECK(false == GLib::Compat::GetEnv(testVar).has_value());

	GLib::Compat::SetEnv(testVar, "Value");
	BOOST_TEST("Value" == *GLib::Compat::GetEnv(testVar));

	GLib::Compat::SetEnv(testVar, "NuValue");
	BOOST_TEST("NuValue" == *GLib::Compat::GetEnv(testVar));

	GLib::Compat::SetEnv(testVar, "Euro \xE2\x82\xAC");
	BOOST_TEST("Euro \xE2\x82\xAC" == *GLib::Compat::GetEnv(testVar));

	auto valueLongerThanDefaultBuffer = std::string(300, '-');
	GLib::Compat::SetEnv(testVar, valueLongerThanDefaultBuffer.c_str());
	BOOST_TEST(valueLongerThanDefaultBuffer == *GLib::Compat::GetEnv(testVar));

	GLib::Compat::UnsetEnv(testVar);
	BOOST_CHECK(false == GLib::Compat::GetEnv(testVar).has_value());
}

BOOST_AUTO_TEST_SUITE_END()