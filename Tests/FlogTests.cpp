
#include "GLib/flogging.h"
#include "GLib/scope.h"

#include <boost/test/unit_test.hpp>

#include <fstream>

BOOST_AUTO_TEST_SUITE(FlogTests)

	struct Fred {};

	BOOST_AUTO_TEST_CASE(BasicTest)
	{
		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Hello");
		log.Info("Format: {0} , {1}", 1, 2);

		std::ifstream in(GLib::Flog::LogManager::GetLogPath());
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
		BOOST_TEST(contents.find(" : INFO     : FlogTests::Fred  : Hello") != std::string::npos);
		BOOST_TEST(contents.find(" : INFO     : FlogTests::Fred  : Format: 1 , 2") != std::string::npos);
	}

	BOOST_AUTO_TEST_CASE(SetThreadName)
	{
		GLib::Flog::LogManager::SetThreadName("TestThread");

		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Hello");
		GLib::Flog::LogManager::SetThreadName(nullptr);

		std::ifstream in(GLib::Flog::LogManager::GetLogPath());
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		BOOST_TEST(contents.find(               "] : INFO     : ThreadName       : TestThread") != std::string::npos);
		BOOST_TEST(contents.find(": [ TestThread ] : INFO     : FlogTests::Fred  : Hello") != std::string::npos);
		BOOST_TEST(contents.find(": [ TestThread ] : INFO     : ThreadName       : (null)") != std::string::npos);
	}

	BOOST_AUTO_TEST_CASE(TestOneScope)
	{
		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Start");
		{
			GLib::Flog::ScopeLog scope1(log, GLib::Flog::Level::Info, "Scoop");
			(void) scope1;
		}
		log.Info("End");

		std::ifstream in(GLib::Flog::LogManager::GetLogPath());
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : Start") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : <==> Scoop") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : End") != std::string::npos);
	}

	BOOST_AUTO_TEST_CASE(TestOneScopeWithInnerLog)
	{
		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Start");
		{
			GLib::Flog::ScopeLog scope1(log, GLib::Flog::Level::Info, "Scoop");
			log.Info("Middle");
			(void) scope1;
		}
		log.Info("End");

		std::ifstream in(GLib::Flog::LogManager::GetLogPath());
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : Start") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : ==> Scoop") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : Middle") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : <== Scoop") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : End") != std::string::npos);
	}

	BOOST_AUTO_TEST_CASE(TestNestedScopes)
	{
		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Start");
		GLib::Flog::ScopeLog scope1(log, GLib::Flog::Level::Info, "Scoop1");
		log.Info("s1");
		{
			GLib::Flog::ScopeLog scope2(log, GLib::Flog::Level::Info, "Scoop2", "--");
			log.Info("s2");
			{
				GLib::Flog::ScopeLog scope3(log, GLib::Flog::Level::Info, "Scoop3", "++");
				log.Info("s3");
			}
		}
		log.Info("End");

		std::ifstream in(GLib::Flog::LogManager::GetLogPath());
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : Start") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : ==> Scoop1") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : s1") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  :  --> Scoop2") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : s2") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  :   ++> Scoop3") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : s3") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  :   <++ Scoop3") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  :  <-- Scoop2") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : FlogTests::Fred  : End") != std::string::npos);
	}

	BOOST_AUTO_TEST_CASE(TestInterlevedLogs)
	{
		auto log1 = GLib::Flog::LogManager::GetLog("Jim");
		auto log2 = GLib::Flog::LogManager::GetLog("Sheila");
		log1.Info("1");
		log2.Info("2");

		std::ifstream in(GLib::Flog::LogManager::GetLogPath());
		std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

		BOOST_TEST(contents.find("] : INFO     : Jim              : 1") != std::string::npos);
		BOOST_TEST(contents.find("] : INFO     : Sheila           : 2") != std::string::npos);
	}

	BOOST_AUTO_TEST_CASE(TestPendingScopeOverPrefixChange)
	{
		auto log1 = GLib::Flog::LogManager::GetLog("Jim");
		auto log2 = GLib::Flog::LogManager::GetLog("Sheila");
		GLib::Flog::ScopeLog scope1(log1, GLib::Flog::Level::Info, "Scoop1");
		{
			GLib::Flog::ScopeLog scope2(log2, GLib::Flog::Level::Info, "Scoop2");
		}
	}

BOOST_AUTO_TEST_SUITE_END()