
#include <GLib/CheckedCast.h>

#include <boost/test/unit_test.hpp>

#include "TestUtils.h"

using GLib::Util::CheckedCast;

namespace
{
	using Exception = std::runtime_error;

	bool IsOverflow(Exception const & e)
	{
		return e.what() == std::string("Overflow");
	}

	bool IsUnderflow(Exception const & e)
	{
		return e.what() == std::string("Underflow");
	}
}

AUTO_TEST_SUITE(CheckedCastTests)

AUTO_TEST_CASE(SameType)
{
	TEST(std::numeric_limits<int>::max() == CheckedCast<int>(std::numeric_limits<int>::max()));
	TEST(std::numeric_limits<int>::min() == CheckedCast<int>(std::numeric_limits<int>::min()));
}

AUTO_TEST_CASE(SameSignSubranged)
{
	TEST(static_cast<short>(1234) == CheckedCast<short>(1234));

	CHECK_EXCEPTION(CheckedCast<short>(std::numeric_limits<int>::min()), Exception, IsUnderflow);

	CHECK_EXCEPTION(CheckedCast<short>(std::numeric_limits<int>::max()), Exception, IsOverflow);
}

AUTO_TEST_CASE(SignedToUnsignedReverseSubranged)
{
	TEST(static_cast<unsigned short>(1234) == CheckedCast<unsigned short>(1234));

	CHECK_EXCEPTION(CheckedCast<unsigned short>(std::numeric_limits<int>::min()), Exception, IsUnderflow);

	CHECK_EXCEPTION(CheckedCast<unsigned short>(std::numeric_limits<int>::max()), Exception, IsOverflow);
}

AUTO_TEST_CASE(SignedToUnsignedNotReverseSubranged)
{
	TEST(1234U == CheckedCast<unsigned int>(static_cast<short>(1234)));
	TEST(static_cast<unsigned int>(std::numeric_limits<short>::max()) == CheckedCast<unsigned int>(std::numeric_limits<short>::max()));

	CHECK_EXCEPTION(CheckedCast<unsigned int>(std::numeric_limits<short>::min()), Exception, IsUnderflow);
}

AUTO_TEST_CASE(UnsignedToSignedSubranged)
{
	TEST(1234 == CheckedCast<int>(1234U));

	CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<unsigned int>::max()), Exception, IsOverflow);
}

AUTO_TEST_CASE(UnsignedToSignedNotSubranged)
{
	TEST(1234 == CheckedCast<int>(static_cast<unsigned short>(1234)));
}

AUTO_TEST_CASE(IntCombos)
{
	TEST(0 == CheckedCast<int>(0));
	TEST(std::numeric_limits<int>::min() == CheckedCast<int>(std::numeric_limits<int>::min()));
	TEST(std::numeric_limits<int>::max() == CheckedCast<int>(std::numeric_limits<int>::max()));

	TEST(static_cast<int>(std::numeric_limits<short>::min()) == CheckedCast<int>(std::numeric_limits<short>::min()));
	TEST(static_cast<int>(std::numeric_limits<short>::max()) == CheckedCast<int>(std::numeric_limits<short>::max()));

	CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<unsigned int>::max()), Exception, IsOverflow);

	CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<long long>::max()), Exception, IsOverflow);

	CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<unsigned long long>::max()), Exception, IsOverflow);

	CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<long long>::min()), Exception, IsUnderflow);
}

AUTO_TEST_SUITE_END()