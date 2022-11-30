
#include <GLib/Scope.h>

#include <boost/test/unit_test.hpp>

#include "TestUtils.h"

AUTO_TEST_SUITE(ScopeTests)

AUTO_TEST_CASE(SimpleTest)
{
	bool deScoped = false;
	{
		auto const scope = GLib::Detail::Scope([&]() noexcept { deScoped = true; });
		TEST(!deScoped);
		static_cast<void>(scope);
	}
	TEST(deScoped);
}

AUTO_TEST_CASE(TwoScopes)
{
	int deScoped = 0;
	{
		auto const scope1 = GLib::Detail::Scope([&]() noexcept { deScoped += 1; });
		auto const scope2 = GLib::Detail::Scope([&]() noexcept { deScoped += 2; });
		TEST(0 == deScoped);
		static_cast<void>(scope2);
		static_cast<void>(scope1);
	}
	TEST(3 == deScoped);
}

AUTO_TEST_SUITE_END()