
#include <GLib/Compat.h>
#include <GLib/Scope.h>

#include <boost/test/unit_test.hpp>

#include "TestUtils.h"

struct UnmangleStruct
{};

class UnmangleClass
{};

AUTO_TEST_SUITE(CompatTests)

AUTO_TEST_CASE(LocalTimeZero)
{
	auto const tz = GLib::Compat::GetEnv("TZ");
	GLib::Compat::SetEnv("TZ", "GMST0GMDT-1,M3.3.0,M10.1.0");
	GLib::Compat::TzSet();

	auto const scope = GLib::Detail::Scope(
		[&]
		{
			if (tz)
			{
				GLib::Compat::SetEnv("TZ", tz->c_str());
			}
			else
			{
				GLib::Compat::UnsetEnv("TZ");
			}
			GLib::Compat::TzSet();
		});

	tm tm {};
	GLib::Compat::LocalTime(tm, 0);
	static_cast<void>(scope);

	TEST(tm.tm_sec == 0);
	TEST(tm.tm_min == 0);
	TEST(tm.tm_hour == 0);
	TEST(tm.tm_mday == 1);
	TEST(tm.tm_mon == 0);
	TEST(tm.tm_year == 70);
	TEST(tm.tm_wday == 4);
	TEST(tm.tm_yday == 0);
	TEST(tm.tm_isdst == 0);
}

AUTO_TEST_CASE(LocalTimeSpecific)
{
	auto const tz = GLib::Compat::GetEnv("TZ");
	GLib::Compat::SetEnv("TZ", "CST");
	GLib::Compat::TzSet();

	auto const scope = GLib::Detail::Scope(
		[&]
		{
			if (tz)
			{
				GLib::Compat::SetEnv("TZ", tz->c_str());
			}
			else
			{
				GLib::Compat::UnsetEnv("TZ");
			}
			GLib::Compat::TzSet();
		});

	tm tm {};
	auto constexpr testTime = 1569056285;
	GLib::Compat::LocalTime(tm, testTime);
	static_cast<void>(scope);

	TEST(tm.tm_sec == 5);
	TEST(tm.tm_min == 58);
	TEST(tm.tm_hour == 8);
	TEST(tm.tm_mday == 21);
	TEST(tm.tm_mon == 8);
	TEST(tm.tm_year == 119);
	TEST(tm.tm_wday == 6);
	TEST(tm.tm_yday == 263);
	TEST(tm.tm_isdst == 0);
}

AUTO_TEST_CASE(GmTime)
{
	tm tm {};
	GLib::Compat::GmTime(tm, 0);

	TEST(tm.tm_sec == 0);
	TEST(tm.tm_min == 0);
	TEST(tm.tm_hour == 0);
	TEST(tm.tm_mday == 1);
	TEST(tm.tm_mon == 0);
	TEST(tm.tm_year == 70);
	TEST(tm.tm_wday == 4);
	TEST(tm.tm_yday == 0);
	TEST(tm.tm_isdst == 0);
}

AUTO_TEST_CASE(StrError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ GLib::Compat::AssertTrue(false, "error", ENOENT); }, "error : No such file or directory");

	GLIB_CHECK_RUNTIME_EXCEPTION({ GLib::Compat::AssertTrue(false, "error", ESRCH); }, "error : No such process");

	GLib::Compat::AssertTrue(true, "no error", 0);
}

AUTO_TEST_CASE(Unmangle)
{
	TEST("UnmangleStruct" == GLib::Compat::Unmangle(typeid(UnmangleStruct).name()));
	TEST("UnmangleClass" == GLib::Compat::Unmangle(typeid(UnmangleClass).name()));
}

AUTO_TEST_CASE(ProcessName)
{
	auto name = GLib::Compat::ProcessName();
	auto const exe = name.rfind(".exe");
	if (exe == name.size() - 4)
	{
		name.erase(exe);
	}
	TEST("Tests" == name);
}

AUTO_TEST_CASE(CommandLine)
{
	if (GLib::Compat::CommandLine().find(GLib::Compat::ProcessName()) == std::string::npos)
	{
		FAIL(GLib::Compat::CommandLine() + " -- " + GLib::Compat::ProcessName());
	}
}

AUTO_TEST_CASE(Env)
{
	auto const * testVar = "TestVar";
	CHECK(false == GLib::Compat::GetEnv(testVar).has_value());

	GLib::Compat::SetEnv(testVar, "Value");
	TEST("Value" == *GLib::Compat::GetEnv(testVar));

	GLib::Compat::SetEnv(testVar, "NuValue");
	TEST("NuValue" == *GLib::Compat::GetEnv(testVar));

	GLib::Compat::SetEnv(testVar, "Euro \xE2\x82\xAC");
	TEST("Euro \xE2\x82\xAC" == *GLib::Compat::GetEnv(testVar));

	auto constexpr size = 300;
	auto valueLongerThanDefaultBuffer = std::string(size, '-');
	GLib::Compat::SetEnv(testVar, valueLongerThanDefaultBuffer.c_str());
	TEST(valueLongerThanDefaultBuffer == *GLib::Compat::GetEnv(testVar));

	GLib::Compat::UnsetEnv(testVar);
	CHECK(false == GLib::Compat::GetEnv(testVar).has_value());
}

AUTO_TEST_SUITE_END()
