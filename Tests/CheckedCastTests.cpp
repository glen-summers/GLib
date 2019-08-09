#include <boost/test/unit_test.hpp>

#include "GLib/checked_cast.h"

using namespace GLib::Util;

namespace
{
	using Exception = std::runtime_error;
	bool IsOverflow(const Exception & e) { return e.what() == std::string("Overflow"); }
	bool IsUnderflow(const Exception & e) { return e.what() == std::string("Underflow"); }
}

BOOST_AUTO_TEST_SUITE(CheckedCastTests)

BOOST_AUTO_TEST_CASE(DetailTest)
{
	BOOST_TEST(true == (Detail::Flags<int, short>::SameSign));
	BOOST_TEST(true == (Detail::Flags<short, int>::SameSign));
	BOOST_TEST(true == (Detail::Flags<int, unsigned int>::TargetSmaller));
	BOOST_TEST(false == (Detail::Flags<unsigned int, int>::SameSign));
	BOOST_TEST(false == (Detail::Flags<int, unsigned int>::SameSign));
	BOOST_TEST(false == (Detail::Flags<int, short>::TargetSmaller));
	BOOST_TEST(false == (Detail::Flags<int, unsigned short>::TargetSmaller));
}

BOOST_AUTO_TEST_CASE(SameType)
{
	BOOST_TEST(std::numeric_limits<int>::max() == GLib::Util::checked_cast<int>(std::numeric_limits<int>::max()));
	BOOST_TEST(std::numeric_limits<int>::min() == GLib::Util::checked_cast<int>(std::numeric_limits<int>::min()));
}

BOOST_AUTO_TEST_CASE(SameSignSubranged)
{
	BOOST_TEST(static_cast<short>(1234) == checked_cast<short>(1234));

	BOOST_CHECK_EXCEPTION(checked_cast<short>(std::numeric_limits<int>::min()), Exception,
		IsUnderflow);

	BOOST_CHECK_EXCEPTION(checked_cast<short>(std::numeric_limits<int>::max()), Exception,
		IsOverflow);
}

BOOST_AUTO_TEST_CASE(SignedToUnsignedReverseSubranged)
{
	BOOST_TEST(static_cast<unsigned short>(1234) == GLib::Util::checked_cast<unsigned short>(1234));

	BOOST_CHECK_EXCEPTION(checked_cast<unsigned short>(std::numeric_limits<int>::min()), Exception,
		IsUnderflow);
	
	BOOST_CHECK_EXCEPTION(checked_cast<unsigned short>(std::numeric_limits<int>::max()), Exception,
		IsOverflow);
}

BOOST_AUTO_TEST_CASE(SignedToUnsignedNotReverseSubranged)
{
	BOOST_TEST(1234u == checked_cast<unsigned int>(static_cast<short>(1234)));
	BOOST_TEST(static_cast<unsigned int>(std::numeric_limits<short>::max()) == checked_cast<unsigned int>(std::numeric_limits<short>::max()));

	BOOST_CHECK_EXCEPTION(checked_cast<unsigned int>(std::numeric_limits<short>::min()), Exception,
		IsUnderflow);
}

BOOST_AUTO_TEST_CASE(UnsignedToSignedSubranged)
{
	BOOST_TEST(1234 == checked_cast<int>(1234u));

	BOOST_CHECK_EXCEPTION(checked_cast<int>(std::numeric_limits<unsigned int>::max()), Exception,
		IsOverflow);
}

BOOST_AUTO_TEST_CASE(UnsignedToSignedNotSubranged)
{
	BOOST_TEST(1234 == checked_cast<int>(static_cast<unsigned short>(1234)));
}

BOOST_AUTO_TEST_CASE(IntCombos)
{
	BOOST_TEST(0 == checked_cast<int>(0));
	BOOST_TEST(std::numeric_limits<int>::min() == checked_cast<int>(std::numeric_limits<int>::min()));
	BOOST_TEST(std::numeric_limits<int>::max() ==checked_cast<int>(std::numeric_limits<int>::max()));

	BOOST_TEST(static_cast<int>(std::numeric_limits<short>::min()) == checked_cast<int>(std::numeric_limits<short>::min()));
	BOOST_TEST(static_cast<int>(std::numeric_limits<short>::max()) == checked_cast<int>(std::numeric_limits<short>::max()));

	BOOST_CHECK_EXCEPTION(checked_cast<int>(std::numeric_limits<unsigned int>::max()), Exception,
		IsOverflow);

	BOOST_CHECK_EXCEPTION(checked_cast<int>(std::numeric_limits<long long>::max()), Exception,
		IsOverflow);

	BOOST_CHECK_EXCEPTION(checked_cast<int>(std::numeric_limits<unsigned long long>::max()), Exception,
		IsOverflow);

	BOOST_CHECK_EXCEPTION(checked_cast<int>(std::numeric_limits<long long>::min()), Exception,
		IsUnderflow);
}

BOOST_AUTO_TEST_SUITE_END()