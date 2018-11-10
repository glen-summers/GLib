
#include "GLib/flogging.h"

#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_SUITE(FlogTests)

	struct Fred {};

	BOOST_AUTO_TEST_CASE(Info)
	{
		std::cout << "Path:" << GLib::Compat::ProcessPath() << std::endl;
		std::cout << "Name:" << GLib::Compat::ProcessName() << std::endl;
		std::cout << "Pid:" << GLib::Compat::ProcessId() << std::endl;
	}

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