
#include "GLib/flogging.h"

#include <boost/test/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(FlogTests)

	struct Fred {};

	BOOST_AUTO_TEST_CASE(BasicTest)
	{
		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Hello");
		log.Info("Format: {0} , {1}", 1, 2);
		// assert file exists and contains...
		BOOST_TEST(true);
	}

	BOOST_AUTO_TEST_CASE(SetThreadName)
	{
		GLib::Flog::LogManager::SetThreadName("TestThread");
		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Hello");
		// assert file exists and contains...
		BOOST_TEST(true);

		// finally
		GLib::Flog::LogManager::SetThreadName(nullptr);
	}

BOOST_AUTO_TEST_SUITE_END()