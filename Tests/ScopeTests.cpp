
#include <GLib/Scope.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ScopeTests)

BOOST_AUTO_TEST_CASE(SimpleTest)
{
	bool deScoped = false;
	{
		auto scope = GLib::Detail::Scope([&]() noexcept { deScoped = true; });
		BOOST_TEST(!deScoped);
		static_cast<void>(scope);
	}
	BOOST_TEST(deScoped);
}

BOOST_AUTO_TEST_CASE(TwoScopes)
{
	int deScoped = 0;
	{
		auto scope1 = GLib::Detail::Scope([&]() noexcept { deScoped += 1; });
		auto scope2 = GLib::Detail::Scope([&]() noexcept { deScoped += 2; });
		BOOST_TEST(0 == deScoped);
		static_cast<void>(scope2);
		static_cast<void>(scope1);
	}
	BOOST_TEST(3 == deScoped);
}

BOOST_AUTO_TEST_SUITE_END()