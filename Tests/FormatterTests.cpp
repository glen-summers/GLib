
#include <GLib/Formatter.h>

#include <boost/test/unit_test.hpp>

#include <chrono>

#include "Xyzzy.h"

#include "TestUtils.h"

namespace
{
	bool IsInvalidFormat(std::logic_error const & e)
	{
		return e.what() == std::string("Invalid format string");
	}

	bool IsNoArguments(std::logic_error const & e)
	{
		return e.what() == std::string("NoArguments");
	}

	auto const UnitedKingdomLocale =
#ifdef __linux__
		"en_GB.UTF8";
#elif _WIN32
		"en-GB";
#else
#endif

	tm MakeTm(int const second, int const minute, int const hour, int const day, int const month, int const year, int const weekDay, int const yearDay,
						int const isDaylightSaving)
	{
		return {second,
						minute,
						hour,
						day,
						month,
						year,
						weekDay,
						yearDay,
						isDaylightSaving
#ifdef __linux__
						,
						0,
						0
#endif
		};
	}
}

using GLib::Formatter;

AUTO_TEST_SUITE(FormatterTests)

AUTO_TEST_CASE(BasicTest)
{
	std::string str = Formatter::Format("{0} {1} {2} {3}", 1, "2", std::string("3"),
																			reinterpret_cast<void *>(4)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	if constexpr (sizeof(void *) == SZ(8))
	{
		TEST(str == "1 2 3 0000000000000004");
	}
	else if constexpr (sizeof(void *) == SZ(4))
	{
		TEST(str == "1 2 3 00000004");
	}
	else
	{
		throw std::runtime_error("unexpected pointer size");
	}
}

AUTO_TEST_CASE(TestEmptyFormatOk)
{
	std::string const str = Formatter::Format("", 1, 2, 3);
	TEST(str.empty());
}

AUTO_TEST_CASE(TestEscapes)
{
	TEST(GLib::FormatterDetail::IsFormattable<Xyzzy>::value);

	std::string str = Formatter::Format("{0} {{1}} {2} {3:{{4}}}", "a", "b", "c", Xyzzy());
	TEST("a {1} c {4}:plover" == str);
}

AUTO_TEST_CASE(NoArgumentsThrows)
{
	CHECK_EXCEPTION(Formatter::Format("{0}"), std::logic_error, IsNoArguments);
}

AUTO_TEST_CASE(TestInvalidEscapeThrows)
{
	CHECK_EXCEPTION(Formatter::Format("{0}}", 0), std::logic_error, IsInvalidFormat);
}

AUTO_TEST_CASE(TestInvalidEndBrace)
{
	CHECK_EXCEPTION(Formatter::Format("xyz {", 0), std::logic_error, IsInvalidFormat);
}

AUTO_TEST_CASE(TestInvalidIndexSpecifier)
{
	CHECK_EXCEPTION(Formatter::Format("{x}", 0), std::logic_error, IsInvalidFormat);
	CHECK_EXCEPTION(Formatter::Format("{0,x}", 0), std::logic_error, IsInvalidFormat);
}

AUTO_TEST_CASE(TestInvalidTrailingIndexSpecifier)
{
	CHECK_EXCEPTION(Formatter::Format("{0", 0), std::logic_error, IsInvalidFormat);
	CHECK_EXCEPTION(Formatter::Format("{0,", 0), std::logic_error, IsInvalidFormat);
	CHECK_EXCEPTION(Formatter::Format("{0,-", 0), std::logic_error, IsInvalidFormat);
	CHECK_EXCEPTION(Formatter::Format("{0,-1", 0), std::logic_error, IsInvalidFormat);
}

AUTO_TEST_CASE(TestInvalidTrailingColon)
{
	CHECK_EXCEPTION(Formatter::Format("{0:", 0), std::logic_error, IsInvalidFormat);
	CHECK_EXCEPTION(Formatter::Format("{0,x}", 0), std::logic_error, IsInvalidFormat);
}

AUTO_TEST_CASE(TestInvalidTrailingFormatBrace)
{
	CHECK_EXCEPTION(Formatter::Format("{0:{", 0), std::logic_error, IsInvalidFormat);
}

AUTO_TEST_CASE(TestInvalidCharacterAfterIndex)
{
	CHECK_EXCEPTION(Formatter::Format("{0x", 0), std::logic_error, IsInvalidFormat);
}

AUTO_TEST_CASE(TestSpacesOk)
{
	std::string str = Formatter::Format("{0    }", I32(1234));
	TEST("1234" == str);

	str = Formatter::Format("{0    ,   -10   }", I32(1234));
	TEST("1234      " == str);
}

AUTO_TEST_CASE(TestRepeatedInsert)
{
	std::string str = Formatter::Format("{0} {0:%x}", I32(1234));
	TEST("1234 4d2" == str);
}

AUTO_TEST_CASE(TestSprintfFormatPassThrough)
{
	std::string str = Formatter::Format("{0:%#.8X}", I32(1234));
	TEST("0X000004D2" == str);
}

AUTO_TEST_CASE(TestSprintfFormatException)
{
	CHECK_EXCEPTION(Formatter::Format("{0:x}", I32(1234)), std::logic_error,
									[](std::logic_error const & e) { return e.what() == std::string("Invalid format : 'x' for Type: int"); });
}

AUTO_TEST_CASE(TestSprintfFormatLengthNoException)
{
	std::string str = Formatter::Format("{0:%#40.8x}", I32(1234));
	TEST("                              0x000004d2" == str);
}

AUTO_TEST_CASE(TestCharLiteral)
{
	std::string str = Formatter::Format("{0}", "abcd");
	TEST("abcd" == str);
}

AUTO_TEST_CASE(TestString)
{
	std::string const value = "abcd";
	std::string str = Formatter::Format("{0}", value);
	TEST("abcd" == str);
}

AUTO_TEST_CASE(TestBool)
{
	std::string str = Formatter::Format("{0}:{1}", true, false);
	TEST("1:0" == str);

	std::ostringstream stm;
	stm << std::boolalpha;
	Formatter::Format(stm, "{0}:{1}", true, false);
	TEST("true:false" == stm.str());
}

AUTO_TEST_CASE(TestInt)
{
	std::string str = Formatter::Format("{0}", -I32(2000000000));
	TEST("-2000000000" == str);
}

AUTO_TEST_CASE(TestIntFormat)
{
	std::string str = Formatter::Format("{0:%x}", -I32(2000000000));
	TEST("88ca6c00" == str);
}

AUTO_TEST_CASE(TestUnisgnedInt)
{
	std::string str = Formatter::Format("{0}", U32(4000000000));
	TEST("4000000000" == str);
}

AUTO_TEST_CASE(TestUnisgnedIntFormat)
{
	std::string str = Formatter::Format("{0:%x}", H32(12345678));
	TEST("12345678" == str);
}

AUTO_TEST_CASE(TestLong)
{
	std::string str = Formatter::Format("{0}", I64(1234567890123456789));
	TEST("1234567890123456789" == str);
}

AUTO_TEST_CASE(TestLongFormat)
{
	std::string str = Formatter::Format("{0:%lx}", H32(12345678));
	TEST("12345678" == str);
}

AUTO_TEST_CASE(TestUnsignedLong)
{
	std::string str = Formatter::Format("{0}", U64(1234567890123456789));
	TEST("1234567890123456789" == str);
}

AUTO_TEST_CASE(TestUnsignedLongFormat)
{
	std::string str = Formatter::Format("{0:%lx}", H32(12345678));
	TEST("12345678" == str);
}

AUTO_TEST_CASE(TestLongLong)
{
	std::string str = Formatter::Format("{0}", I64(1234567890123456789));
	TEST("1234567890123456789" == str);
}

AUTO_TEST_CASE(TestLongLongFormat)
{
	std::string str = Formatter::Format("{0:%llx}", H64(123456789abcdef0));
	TEST("123456789abcdef0" == str);
}

AUTO_TEST_CASE(TestULongLong)
{
	std::string str = Formatter::Format("{0}", U64(12345678901234567890));
	TEST("12345678901234567890" == str);
}

AUTO_TEST_CASE(TestULongLongFormat)
{
	std::string str = Formatter::Format("{0:%llx}", H64(123456789abcdef0));
	TEST("123456789abcdef0" == str);
}

AUTO_TEST_CASE(TestFloat)
{
	auto foo = FL(1.234);

	std::string str = Formatter::Format("{0}", foo);
	TEST("1.234" == str);
}

AUTO_TEST_CASE(TestFloatFormat)
{
	std::string str = Formatter::Format("{0:%.2f}", FL(1.2345678));
	TEST("1.23" == str);
}

AUTO_TEST_CASE(TestDouble)
{
	std::string str = Formatter::Format("{0}", DB(1.234));
	TEST("1.234" == str);
}

AUTO_TEST_CASE(TestDoubleFormat)
{
	std::string str = Formatter::Format("{0:%.2f}", DB(1.23456789012345));
	TEST("1.23" == str);
}

AUTO_TEST_CASE(TestLongDouble)
{
	std::string str = Formatter::Format("{0}", LD(1.234));
	TEST("1.234" == str);
}

AUTO_TEST_CASE(TestLongDoubleFormat)
{
	std::string str = Formatter::Format("{0:%.2Lf}", LD(1.23456789012345));
	TEST("1.23" == str);
}

AUTO_TEST_CASE(TestPointer32)
{
	if constexpr (sizeof(void *) == sizeof(uint32_t))
	{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4312) // x64 -> warning C4312: 'type cast': conversion from 'T' to 'const void *' of greater size
#endif

#ifdef __GNUG__
#pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Werror=int-to-pointer-cast"
#endif

		// -Werror=int-to-pointer-cast
		auto const * p = reinterpret_cast<void const *>(H32(1234abcd)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)

		std::string str = Formatter::Format("{0}", p);
		TEST("1234ABCD" == str);
	}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __GNUG__
#pragma GCC diagnostic pop
#endif
}

AUTO_TEST_CASE(TestPointer64)
{
	if constexpr (sizeof(void *) == sizeof(uint64_t))
	{
		auto * p = reinterpret_cast<void *>(H64(123456789abcdef0)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast, performance-no-int-to-ptr)
		std::string str = Formatter::Format("{0}", p);
		TEST("123456789ABCDEF0" == str);
	}
}

AUTO_TEST_CASE(TestTimePointDefaultFormat)
{
	int constexpr offset = 1900;
	int constexpr yr = 1601;
	tm const tm = MakeTm(0, 0, 0, 1, 0, yr - offset, 0, 0, 0);

	std::ostringstream str;
	str.imbue(std::locale(UnitedKingdomLocale));
	Formatter::Format(str, "{0}", tm);
	TEST("01 Jan 1601, 00:00:00" == str.str());
}

AUTO_TEST_CASE(TestTimePoint)
{
	tm const tm = MakeTm(0, 0, 18, 6, 10, 67, 0, 0, 1);

	std::ostringstream str;
	str.imbue(std::locale(UnitedKingdomLocale));
	Formatter::Format(str, "{0:%d %b %Y, %H:%M:%S}", tm);
	TEST("06 Nov 1967, 18:00:00" == str.str());
}

AUTO_TEST_CASE(TestPad)
{
	std::string str = Formatter::Format("{0},{1,4},{2}", 0, 1, 2);
	TEST("0,   1,2" == str);

	str = Formatter::Format("{0},{1,-4},{2}", 0, 1, 2);
	TEST("0,1   ,2" == str);
}

AUTO_TEST_CASE(CustomTypeNoFormat)
{
	Xyzzy const plugh;
	std::string str = Formatter::Format("{0}", plugh);
	TEST("plover" == str);
}

AUTO_TEST_CASE(CustomTypeFormat)
{
	Xyzzy const plugh;
	std::string str = Formatter::Format("{0:fmt}", plugh);
	TEST("fmt:plover" == str);
}

AUTO_TEST_CASE(CustomTypeNonEmptyFormatException)
{
	CHECK_EXCEPTION(Xyzzy2 plugh; Formatter::Format("{0:lentilCustard}", plugh), std::logic_error,
																[](std::logic_error const & e) { return e.what() == std::string("Unexpected non-empty format : lentilCustard"); });
}

AUTO_TEST_CASE(MoneyTest)
{
	GLib::Money const m {FL(123456.7)};

	std::ostringstream str;
	str.imbue(std::locale(UnitedKingdomLocale));
	Formatter::Format(str, "{0}", m);
	auto const * expected = "\xc2\xa3"
													"1,234.57";
	TEST(expected == str.str());
}

AUTO_TEST_CASE(TestLargeObject)
{
	CopyCheck const c1;
	CHECK(c1.Copies() == 0 && c1.Moves() == 0);

	CopyCheck const c2(c1); // NOLINT(performance-unnecessary-copy-initialization) test copy
	CHECK(c2.Copies() == 1 && c1.Moves() == 0);

	CopyCheck const c3;
	std::string str = Formatter::Format("{0}", c3);
	TEST("0:0" == str);

	str = Formatter::Format("{0}", CopyCheck());
	TEST("0:0" == str);
}

AUTO_TEST_SUITE_END()
