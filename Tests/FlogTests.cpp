
#include "../GLib/DurationPrinter.h"

#include <GLib/flogging.h>
#include <GLib/formatter.h>
#include <GLib/scope.h>

#include <boost/test/unit_test.hpp>

#include <fstream>

std::string ToString(const std::chrono::nanoseconds & duration)
{
	std::ostringstream s;
	s << duration;
	return s.str();
}

BOOST_AUTO_TEST_SUITE(FlogTests)

BOOST_AUTO_TEST_CASE(Duration)
{
	BOOST_TEST("578.9ms" == ToString(std::chrono::microseconds {578912}));
	BOOST_TEST("0:0:1.234" == ToString(std::chrono::milliseconds {1234}));
	BOOST_TEST("0:34:13" == ToString(std::chrono::seconds {34 * 60 + 13}));
	BOOST_TEST("9:34:13" == ToString(std::chrono::seconds {9 * 3600 + 34 * 60 + 13}));
	BOOST_TEST("1.9:34:13" == ToString(std::chrono::seconds {33 * 3600 + 34 * 60 + 13}));
}

struct Fred
{};

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

BOOST_AUTO_TEST_CASE(LogLevel)
{
	auto log = GLib::Flog::LogManager::GetLog<Fred>();

	auto currentLevel = GLib::Flog::LogManager::SetLevel(GLib::Flog::Level::Error);
	SCOPE(_, [=]() { GLib::Flog::LogManager::SetLevel(currentLevel); });

	log.Info("info");
	log.Error("error");

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	BOOST_TEST(contents.find(" : INFO     : FlogTests::Fred  : info") == std::string::npos);
	BOOST_TEST(contents.find(" : ERROR    : FlogTests::Fred  : error") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(LogFileSize)
{
	auto log = GLib::Flog::LogManager::GetLog<Fred>();

	log.Info("Start");
	auto path1 = GLib::Flog::LogManager::GetLogPath();
	auto currentSize = GLib::Flog::LogManager::SetMaxFileSize(1024);
	SCOPE(_, [=]() { GLib::Flog::LogManager::SetMaxFileSize(currentSize); });

	log.Info(std::string(100, 'x'));
	BOOST_TEST(path1 == GLib::Flog::LogManager::GetLogPath());

	log.Info(std::string(924, 'x'));
	BOOST_TEST(path1 != GLib::Flog::LogManager::GetLogPath());
}

BOOST_AUTO_TEST_CASE(ProcessName)
{
	{
		auto log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Hello");
	}

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	auto processName = GLib::Compat::ProcessName();
	auto processPath = GLib::Compat::ProcessPath();
	auto bitness = std::to_string(8 * sizeof(void *));

	BOOST_TEST(contents.find("ProcessName : (" + bitness + " bit) " + processName) != std::string::npos);
	BOOST_TEST(contents.find("FullPath    : " + processPath) != std::string::npos);
}

BOOST_AUTO_TEST_CASE(SetThreadName)
{
	GLib::Flog::LogManager::SetThreadName("TestThread");

	auto log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Hello");
	GLib::Flog::LogManager::SetThreadName({});

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	BOOST_TEST(contents.find("] : INFO     : ThreadName       : TestThread") != std::string::npos);
	BOOST_TEST(contents.find(": [ TestThread ] : INFO     : FlogTests::Fred  : Hello") != std::string::npos);
	BOOST_TEST(contents.find(": [ TestThread ] : INFO     : ThreadName       : (null)") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(TestOneScope)
{
	auto log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Start");
	{
		GLib::Flog::ScopeLog scope1(log, GLib::Flog::Level::Info, "Scoop");
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