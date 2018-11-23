
#include "GLib/formatter.h"
#if _WIN32
#include "GLib/Win/DebugStream.h"
#endif
#include "Xyzzy.h"

#include <boost/test/unit_test.hpp>

#include <chrono>


namespace
{
	bool IsInvalidFormat(const std::logic_error & e) { return e.what() == std::string("Invalid format string"); }
	bool IsIndexOutOfRange(const std::logic_error & e) { return e.what() == std::string("IndexOutOfRange"); }

	const auto ukLocale =
#ifdef __linux__
		"en_GB.UTF8";
#elif _WIN32
		"en-GB";
#else
#endif

	tm MakeTm(int tm_sec, int tm_min, int tm_hour, int tm_mday, int tm_mon, int tm_year, int tm_wday, int tm_yday, int tm_isdst)
	{
		return { tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst
#ifdef __linux__
			,0,0
#endif
		};
	}
}

using GLib::Formatter;

BOOST_AUTO_TEST_SUITE(FormatterTests)

BOOST_AUTO_TEST_CASE(BasicTest)
{
	std::string s = Formatter::Format("{0} {1} {2} {3}", 1, "2", std::string("3"), reinterpret_cast<void*>(4));
	if constexpr (sizeof(void*) == 8)
	{
		BOOST_TEST(s == "1 2 3 0000000000000004");
	}
	else if constexpr (sizeof(void*) == 4)
	{
		BOOST_TEST(s == "1 2 3 00000004");
	}
	else
	{
		throw std::runtime_error("unexpected pointer size");
	}
}

BOOST_AUTO_TEST_CASE(TestEmptyFormatOk)
{
	std::string s = Formatter::Format("", 1, 2, 3);
	BOOST_TEST(s.empty());
}

BOOST_AUTO_TEST_CASE(TestEscapes)
{
	BOOST_TEST(GLib::FormatterDetail::IsFormattable<Xyzzy>::value);

	std::string s = Formatter::Format("{0} {{1}} {2} {3:{{4}}}", "a", "b", "c", Xyzzy());
	BOOST_TEST("a {1} c {4}:plover" == s);
}

BOOST_AUTO_TEST_CASE(TestInvalidEscapeThrows)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0}}", 0), std::logic_error, IsInvalidFormat);
}

BOOST_AUTO_TEST_CASE(TestInvalidEndBrace)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("xyz {"), std::logic_error, IsInvalidFormat);
}

BOOST_AUTO_TEST_CASE(TestInvalidIndexSpecifier)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{x}"), std::logic_error, IsInvalidFormat);
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0,x}", 0), std::logic_error, IsInvalidFormat);
}

BOOST_AUTO_TEST_CASE(TestInvalidTrailingIndexSpecifier)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0", 0), std::logic_error, IsInvalidFormat);
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0,", 0), std::logic_error, IsInvalidFormat);
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0,-", 0), std::logic_error, IsInvalidFormat);
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0,-1", 0), std::logic_error, IsInvalidFormat);
}

BOOST_AUTO_TEST_CASE(TestInvalidTrailingColon)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0:", 0), std::logic_error, IsInvalidFormat);
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0,x}", 0), std::logic_error, IsInvalidFormat);
}

BOOST_AUTO_TEST_CASE(TestInvalidTrailingFormatBrace)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0:{", 0), std::logic_error, IsInvalidFormat);
}

BOOST_AUTO_TEST_CASE(TestInvalidCharacterAfterIndex)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0x", 0), std::logic_error, IsInvalidFormat);
}

BOOST_AUTO_TEST_CASE(TestInvalidIndex)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{1}", 0), std::logic_error, IsIndexOutOfRange);
}

BOOST_AUTO_TEST_CASE(TestSpacesOk)
{
	std::string s = Formatter::Format("{0    }", 1234);
	BOOST_TEST("1234" == s);

	s = Formatter::Format("{0    ,   -10   }", 1234);
	BOOST_TEST("1234      " == s);
}

BOOST_AUTO_TEST_CASE(TestRepeatedInsert)
{
	std::string s = Formatter::Format("{0} {0:%x}", 1234);
	BOOST_TEST("1234 4d2" == s);
}

BOOST_AUTO_TEST_CASE(TestSprintfFormatPassThrough)
{
	std::string s = Formatter::Format("{0:%#.8X}", 1234);
	BOOST_TEST("0X000004D2" ==s);
}

BOOST_AUTO_TEST_CASE(TestSprintfFormatException)
{
	BOOST_CHECK_EXCEPTION(Formatter::Format("{0:x}", 1234), std::logic_error, [](const std::logic_error & e)
	{
		return e.what() == std::string("Invalid format : x");
	});
}

BOOST_AUTO_TEST_CASE(TestSprintfFormatLengthNoException)
{
	std::string s = Formatter::Format("{0:%#40.8x}", 1234);
	BOOST_TEST("                              0x000004d2" == s);
}

BOOST_AUTO_TEST_CASE(TestIntAsFloat)
{
	// produces random stuff
	std::string s = Formatter::Format("{0:%f}", 1234);
	//BOOST_TEST("1234" == s);
}

BOOST_AUTO_TEST_CASE(TestCharLiteral)
{
	std::string s = Formatter::Format("{0}", "abcd");
	BOOST_TEST("abcd" == s);
}

BOOST_AUTO_TEST_CASE(TestString)
{
	std::string value = "abcd";
	std::string s = Formatter::Format("{0}", value);
	BOOST_TEST("abcd" == s);
}

BOOST_AUTO_TEST_CASE(TestBool)
{
	std::string s = Formatter::Format("{0}:{1}", true, false);
	BOOST_TEST("1:0" == s);

	std::ostringstream stm;
	stm << std::boolalpha;
	Formatter::Format(stm, "{0}:{1}", true, false);
	BOOST_TEST("true:false" == stm.str());
}

BOOST_AUTO_TEST_CASE(TestInt)
{
	std::string s = Formatter::Format("{0}", -2000000000);
	BOOST_TEST("-2000000000" == s);
}

BOOST_AUTO_TEST_CASE(TestIntFormat)
{
	std::string s = Formatter::Format("{0:%x}", -2000000000);
	BOOST_TEST("88ca6c00" == s);
}

BOOST_AUTO_TEST_CASE(TestUnisgnedInt)
{
	std::string s = Formatter::Format("{0}", 4000000000U);
	BOOST_TEST("4000000000" == s);
}

BOOST_AUTO_TEST_CASE(TestUnisgnedIntFormat)
{
	std::string s = Formatter::Format("{0:%x}", 0x12345678U);
	BOOST_TEST("12345678" == s);
}

BOOST_AUTO_TEST_CASE(TestLong)
{
	std::string s = Formatter::Format("{0}", 1234567890123456789L);
	BOOST_TEST("1234567890123456789" == s);
}

BOOST_AUTO_TEST_CASE(TestLongFormat)
{
	std::string s = Formatter::Format("{0:%lx}", 0x12345678L);
	BOOST_TEST("12345678" == s);
}

BOOST_AUTO_TEST_CASE(TestUnsignedLong)
{
	std::string s = Formatter::Format("{0}", 1234567890123456789UL);
	BOOST_TEST("1234567890123456789" == s);
}

BOOST_AUTO_TEST_CASE(TestUnsignedLongFormat)
{
	std::string s = Formatter::Format("{0:%lx}", 0x12345678UL);
	BOOST_TEST("12345678" == s);
}

BOOST_AUTO_TEST_CASE(TestLongLong)
{
	std::string s = Formatter::Format("{0}", 1234567890123456789LL);
	BOOST_TEST("1234567890123456789" == s);
}

BOOST_AUTO_TEST_CASE(TestLongLongFormat)
{
	std::string s = Formatter::Format("{0:%llx}", 0x123456789abcfdefLL);
	BOOST_TEST("123456789abcfdef" ==  s);
}

BOOST_AUTO_TEST_CASE(TestULongLong)
{
	std::string s = Formatter::Format("{0}", 12345678901234567890ULL);
	BOOST_TEST("12345678901234567890" ==  s);
}

BOOST_AUTO_TEST_CASE(TestULongLongFormat)
{
	std::string s = Formatter::Format("{0:%llx}", 0x123456789abcfdefULL);
	BOOST_TEST("123456789abcfdef" == s);
}

BOOST_AUTO_TEST_CASE(TestFloat)
{
	std::string s = Formatter::Format("{0}", 1.234f);
	BOOST_TEST("1.234" == s);
}

BOOST_AUTO_TEST_CASE(TestFloatFormat)
{
	std::string s = Formatter::Format("{0:%.2f}", 1.2345678f);
	BOOST_TEST("1.23" == s);
}

BOOST_AUTO_TEST_CASE(TestDouble)
{
	std::string s = Formatter::Format("{0}", 1.234);
	BOOST_TEST("1.234" == s);
}

BOOST_AUTO_TEST_CASE(TestDoubleFormat)
{
	std::string s = Formatter::Format("{0:%.2f}", 1.23456789012345);
	BOOST_TEST("1.23" == s);
}

BOOST_AUTO_TEST_CASE(TestLongDouble)
{
	std::string s = Formatter::Format("{0}", 1.234L);
	BOOST_TEST("1.234" == s);
}

BOOST_AUTO_TEST_CASE(TestLongDoubleFormat)
{
	std::string s = Formatter::Format("{0:%.2Lf}", 1.23456789012345L);
	BOOST_TEST("1.23" == s);
}

BOOST_AUTO_TEST_CASE(TestPointer)
{
	auto p = reinterpret_cast<void*>(0x12345);

	std::string s = Formatter::Format("{0}", p);
	if constexpr (sizeof(void*)==8)
	{
		BOOST_TEST("0000000000012345" == s);
	}
	else if constexpr (sizeof(void*) == 4)
	{
		BOOST_TEST("00012345" == s);
	}
	else
	{
		throw std::runtime_error("unexpected pointer size");
	}
}

BOOST_AUTO_TEST_CASE(TestTimePointDefaultFormat)
{
	const int offset = 1900;
	const int yr = 1601;
	const tm tm = MakeTm( 0,0,0, 1,0, yr - offset, 0,0, 0 );

	std::ostringstream s;
	s.imbue(std::locale(ukLocale));
	Formatter::Format(s, "{0}", tm);
	BOOST_TEST("01 Jan 1601, 00:00:00" == s.str());
}

BOOST_AUTO_TEST_CASE(TestTimePoint)
{
	const tm tm = MakeTm( 0,0,18, 6,10,67, 0,0, 1 );

	std::ostringstream s;
	s.imbue(std::locale(ukLocale));
	Formatter::Format(s, "{0:%d %b %Y, %H:%M:%S}", tm);
	BOOST_TEST("06 Nov 1967, 18:00:00" == s.str());
}

BOOST_AUTO_TEST_CASE(TestPad)
{
	std::string s = Formatter::Format("{0},{1,4},{2}", 0, 1, 2);
	BOOST_TEST("0,   1,2" == s);

	s = Formatter::Format("{0},{1,-4},{2}", 0, 1, 2);
	BOOST_TEST("0,1   ,2" == s);
}

BOOST_AUTO_TEST_CASE(CustomTypeNoFormat)
{
	Xyzzy plugh;
	std::string s = Formatter::Format("{0}", plugh);
	BOOST_TEST("plover" == s);
}

BOOST_AUTO_TEST_CASE(CustomTypeFormat)
{
	Xyzzy plugh;
	std::string s = Formatter::Format("{0:fmt}", plugh);
	BOOST_TEST("fmt:plover" == s);
}

BOOST_AUTO_TEST_CASE(CustomTypeNonEmptyFormatException)
{
	BOOST_CHECK_EXCEPTION(Xyzzy2 plugh; Formatter::Format("{0:lentilCustard}", plugh), std::logic_error, [](const std::logic_error & e)
	{
		return e.what() == std::string("Unexpected non-empty format : lentilCustard");
	});
}

BOOST_AUTO_TEST_CASE(MoneyTest)
{
	GLib::Money m { 123456.7 };

	std::ostringstream s;
	s.imbue(std::locale(ukLocale));
	Formatter::Format(s, "{0}", m);
	auto expected = "\xc2\xa3" "1,234.57";
	BOOST_TEST(expected == s.str());
}

BOOST_AUTO_TEST_CASE(TestLargeObject)
{
	CopyCheck c1;
	BOOST_CHECK(c1.copies == 0 && c1.moves == 0);

	CopyCheck c2(c1);
	BOOST_CHECK(c2.copies == 1 && c1.moves == 0);

	CopyCheck c3;
	std::string s = Formatter::Format("{0}", c3);
	BOOST_TEST("0:0" == s);

	s = Formatter::Format("{0}", CopyCheck());
	BOOST_TEST("0:0" == s);
}

#if _WIN32
BOOST_AUTO_TEST_CASE(DebugStream)
{
	GLib::Win::Debug::Write("DebugStreamTest1");
	GLib::Win::Debug::Write("DebugStreamTest2 {0} {1} {2}", 1, 2, 3);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
