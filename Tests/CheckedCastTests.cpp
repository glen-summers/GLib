
#include <GLib/CheckedCast.h>

#include <boost/test/unit_test.hpp>

using namespace GLib::Util;

namespace
{
	using Exception = std::runtime_error;

	bool IsOverflow(const Exception & e)
	{
		return e.what() == std::string("Overflow");
	}

	bool IsUnderflow(const Exception & e)
	{
		return e.what() == std::string("Underflow");
	}
}

BOOST_AUTO_TEST_SUITE(CheckedCastTests)

BOOST_AUTO_TEST_CASE(SameType)
{
	BOOST_TEST(std::numeric_limits<int>::max() == CheckedCast<int>(std::numeric_limits<int>::max()));
	BOOST_TEST(std::numeric_limits<int>::min() == CheckedCast<int>(std::numeric_limits<int>::min()));
}

BOOST_AUTO_TEST_CASE(SameSignSubranged)
{
	BOOST_TEST(static_cast<short>(1234) == CheckedCast<short>(1234));

	BOOST_CHECK_EXCEPTION(CheckedCast<short>(std::numeric_limits<int>::min()), Exception, IsUnderflow);

	BOOST_CHECK_EXCEPTION(CheckedCast<short>(std::numeric_limits<int>::max()), Exception, IsOverflow);
}

BOOST_AUTO_TEST_CASE(SignedToUnsignedReverseSubranged)
{
	BOOST_TEST(static_cast<unsigned short>(1234) == CheckedCast<unsigned short>(1234));

	BOOST_CHECK_EXCEPTION(CheckedCast<unsigned short>(std::numeric_limits<int>::min()), Exception, IsUnderflow);

	BOOST_CHECK_EXCEPTION(CheckedCast<unsigned short>(std::numeric_limits<int>::max()), Exception, IsOverflow);
}

BOOST_AUTO_TEST_CASE(SignedToUnsignedNotReverseSubranged)
{
	BOOST_TEST(1234u == CheckedCast<unsigned int>(static_cast<short>(1234)));
	BOOST_TEST(static_cast<unsigned int>(std::numeric_limits<short>::max()) == CheckedCast<unsigned int>(std::numeric_limits<short>::max()));

	BOOST_CHECK_EXCEPTION(CheckedCast<unsigned int>(std::numeric_limits<short>::min()), Exception, IsUnderflow);
}

BOOST_AUTO_TEST_CASE(UnsignedToSignedSubranged)
{
	BOOST_TEST(1234 == CheckedCast<int>(1234U));

	BOOST_CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<unsigned int>::max()), Exception, IsOverflow);
}

BOOST_AUTO_TEST_CASE(UnsignedToSignedNotSubranged)
{
	BOOST_TEST(1234 == CheckedCast<int>(static_cast<unsigned short>(1234)));
}

BOOST_AUTO_TEST_CASE(IntCombos)
{
	BOOST_TEST(0 == CheckedCast<int>(0));
	BOOST_TEST(std::numeric_limits<int>::min() == CheckedCast<int>(std::numeric_limits<int>::min()));
	BOOST_TEST(std::numeric_limits<int>::max() == CheckedCast<int>(std::numeric_limits<int>::max()));

	BOOST_TEST(static_cast<int>(std::numeric_limits<short>::min()) == CheckedCast<int>(std::numeric_limits<short>::min()));
	BOOST_TEST(static_cast<int>(std::numeric_limits<short>::max()) == CheckedCast<int>(std::numeric_limits<short>::max()));

	BOOST_CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<unsigned int>::max()), Exception, IsOverflow);

	BOOST_CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<long long>::max()), Exception, IsOverflow);

	BOOST_CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<unsigned long long>::max()), Exception, IsOverflow);

	BOOST_CHECK_EXCEPTION(CheckedCast<int>(std::numeric_limits<long long>::min()), Exception, IsUnderflow);
}

BOOST_AUTO_TEST_SUITE_END()