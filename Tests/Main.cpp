
#define BOOST_TEST_MODULE "GLib C++ Unit Tests"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
#elif __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <boost/test/included/unit_test.hpp>
#ifndef BOOST_TEST
#error boost
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#elif __GNUG__
#pragma GCC diagnostic pop
#endif
