#include <GLib/StackOrHeap.h>

#include <boost/test/unit_test.hpp>

#include <GLib/Scope.h>

#include "TestUtils.h"

constexpr auto sz50 = SZ(50);
constexpr auto sz99 = SZ(99);
constexpr auto sz100 = SZ(100);
constexpr auto sz199 = SZ(199);
constexpr auto sz200 = SZ(200);

AUTO_TEST_SUITE(StackOrHeapTests)

AUTO_TEST_CASE(Alloc)
{
	GLib::Util::StackOrHeap<char, sz100> s;
	CHECK(sz100 == s.Size());
	TEST(nullptr != s.Get());

	auto * ss = new (s.Get()) std::string(sz99, '-'); // NOLINT(cppcoreguidelines-owning-memory)
	TEST(*ss == std::string(sz99, '-'));
	ss->std::string::~string();
}

AUTO_TEST_CASE(Realloc)
{
	GLib::Util::StackOrHeap<char, sz100> s;

	CHECK(sz100 == s.Size());

	s.EnsureSize(sz50);
	CHECK(sz100 == s.Size());

	{
		auto const * ss = new (s.Get()) std::string(sz99, '-'); // NOLINT(cppcoreguidelines-owning-memory)
		auto const scope = GLib::Detail::Scope([&]() { ss->std::string::~string(); });
		TEST(*ss == std::string(sz99, '-'));
		static_cast<void>(scope);
	}

	s.EnsureSize(sz200);
	CHECK(sz200 == s.Size());

	{
		auto const * sss = new (s.Get()) std::string(sz199, '-'); // NOLINT(cppcoreguidelines-owning-memory)
		auto const scope = GLib::Detail::Scope([&]() { sss->std::string::~string(); });
		TEST(*sss == std::string(sz199, '-'));
		static_cast<void>(scope);
	}
}

AUTO_TEST_CASE(Const)
{
	GLib::Util::StackOrHeap<char, sz100> const s;
	CHECK(sz100 == s.Size());
	TEST(nullptr != s.Get());
}

AUTO_TEST_SUITE_END()
