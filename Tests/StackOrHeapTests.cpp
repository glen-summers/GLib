#include <GLib/StackOrHeap.h>

#include <boost/test/unit_test.hpp>

namespace
{
	constexpr auto operator"" _size(unsigned long long int n)
	{
		return static_cast<size_t>(n);
	}
}

BOOST_AUTO_TEST_SUITE(StackOrHeapTests)

BOOST_AUTO_TEST_CASE(Alloc)
{
	GLib::Util::StackOrHeap<char, 100_size> s;
	BOOST_CHECK(100_size == s.Size());
	BOOST_TEST(nullptr != s.Get());

	std::string * ss = new (s.Get()) std::string(99_size, '-');
	BOOST_TEST(*ss == std::string(99_size, '-'));
	ss->std::string::~string();
}

BOOST_AUTO_TEST_CASE(Realloc)
{
	GLib::Util::StackOrHeap<char, 100_size> s;

	BOOST_CHECK(100_size == s.Size());

	s.EnsureSize(50_size);
	BOOST_CHECK(100_size == s.Size());

	{
		std::string * ss = new (s.Get()) std::string(99_size, '-');
		BOOST_TEST(*ss == std::string(99_size, '-'));
		ss->std::string::~string();
	}

	s.EnsureSize(200_size);
	BOOST_CHECK(200_size == s.Size());

	{
		std::string * sss = new (s.Get()) std::string(199_size, '-');
		BOOST_TEST(*sss == std::string(199_size, '-'));
		sss->std::string::~string();
	}
}

BOOST_AUTO_TEST_CASE(Const)
{
	const GLib::Util::StackOrHeap<char, 100_size> s;

	BOOST_CHECK(100_size == s.Size());
	BOOST_TEST(nullptr != s.Get());
}

BOOST_AUTO_TEST_SUITE_END()
