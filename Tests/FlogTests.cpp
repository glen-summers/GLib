
#include "../GLib/DurationPrinter.h"

#include <GLib/Flogging.h>
#include <GLib/Formatter.h>
#include <GLib/Scope.h>

#include <boost/test/unit_test.hpp>

#include <fstream>

#include "TestUtils.h"

std::string ToString(std::chrono::nanoseconds const & duration)
{
	std::ostringstream s;
	s << duration;
	return s.str();
}

AUTO_TEST_SUITE(FlogTests)

AUTO_TEST_CASE(Duration)
{
	TEST("578.9ms" == ToString(std::chrono::microseconds {578912}));
	TEST("0:0:1.2" == ToString(std::chrono::milliseconds {1200}));
	TEST("0:0:1.23" == ToString(std::chrono::milliseconds {1230}));
	TEST("0:0:1.234" == ToString(std::chrono::milliseconds {1234}));
	TEST("0:0:2" == ToString(std::chrono::seconds {2}));
	TEST("0:34:13" == ToString(std::chrono::seconds {34 * 60 + 13}));
	TEST("9:34:13" == ToString(std::chrono::seconds {9 * 3600 + 34 * 60 + 13}));
	TEST("1.9:34:13" == ToString(std::chrono::seconds {33 * 3600 + 34 * 60 + 13}));
}

struct Fred
{};

AUTO_TEST_CASE(BasicTest)
{
	auto const log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Hello");
	log.Info("Format: {0} , {1}", 1, 2);

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());
	TEST(contents.find(" : INFO     : FlogTests::Fred  : Hello") != std::string::npos);
	TEST(contents.find(" : INFO     : FlogTests::Fred  : Format: 1 , 2") != std::string::npos);
}

AUTO_TEST_CASE(LogLevel)
{
	auto const log = GLib::Flog::LogManager::GetLog<Fred>();

	auto const currentLevel = GLib::Flog::LogManager::SetLevel(GLib::Flog::Level::Error);

	auto const scope = GLib::Detail::Scope([=] { GLib::Flog::LogManager::SetLevel(currentLevel); });

	log.Info("info");
	log.Error("error");

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());
	TEST(contents.find(" : INFO     : FlogTests::Fred  : info") == std::string::npos);
	TEST(contents.find(" : ERROR    : FlogTests::Fred  : error") != std::string::npos);

	static_cast<void>(scope);
}

AUTO_TEST_CASE(LogFileSize)
{
	auto const log = GLib::Flog::LogManager::GetLog<Fred>();

	log.Info("Start");
	auto const path1 = GLib::Flog::LogManager::GetLogPath();

	auto const currentSize = GLib::Flog::LogManager::SetMaxFileSize(SZ(1024));

	auto const scope = GLib::Detail::Scope([=] { GLib::Flog::LogManager::SetMaxFileSize(currentSize); });

	log.Info(std::string(SZ(100), 'x'));
	TEST(path1 == GLib::Flog::LogManager::GetLogPath());

	log.Info(std::string(SZ(924), 'x'));
	TEST(path1 != GLib::Flog::LogManager::GetLogPath());

	static_cast<void>(scope);
}

AUTO_TEST_CASE(ProcessName)
{
	{
		auto const log = GLib::Flog::LogManager::GetLog<Fred>();
		log.Info("Hello");
	}

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());

	auto const processName = GLib::Compat::ProcessName();
	auto const processPath = GLib::Compat::ProcessPath();
	auto const bitness = std::to_string(I32(8) * sizeof(void *));

	TEST(contents.find("ProcessName : (" + bitness + " bit) " + processName) != std::string::npos);
	TEST(contents.find("FullPath    : " + processPath) != std::string::npos);
}

AUTO_TEST_CASE(SetThreadName)
{
	GLib::Flog::LogManager::SetThreadName("TestThread");

	auto const log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Hello");
	GLib::Flog::LogManager::SetThreadName({});

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string const contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());

	TEST(contents.find("] : INFO     : ThreadName       : TestThread") != std::string::npos);
	TEST(contents.find(": [ TestThread ] : INFO     : FlogTests::Fred  : Hello") != std::string::npos);
	TEST(contents.find(": [ TestThread ] : INFO     : ThreadName       : (null)") != std::string::npos);
}

AUTO_TEST_CASE(TestOneScope)
{
	auto const log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Start");
	{
		GLib::Flog::ScopeLog const scope1(log, GLib::Flog::Level::Info, "Scoop");
		static_cast<void>(scope1);
	}
	log.Info("End");

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string const contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());

	TEST(contents.find("] : INFO     : FlogTests::Fred  : Start") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : <==> Scoop") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : End") != std::string::npos);
}

AUTO_TEST_CASE(TestOneScopeWithInnerLog)
{
	auto const log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Start");
	{
		GLib::Flog::ScopeLog const scope1(log, GLib::Flog::Level::Info, "Scoop");
		log.Info("Middle");
		static_cast<void>(scope1);
	}
	log.Info("End");

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string const contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());

	TEST(contents.find("] : INFO     : FlogTests::Fred  : Start") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : ==> Scoop") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : Middle") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : <== Scoop") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : End") != std::string::npos);
}

AUTO_TEST_CASE(TestNestedScopes)
{
	auto const log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Start");
	GLib::Flog::ScopeLog const scope1(log, GLib::Flog::Level::Info, "Scoop1");
	log.Info("s1");
	{
		GLib::Flog::ScopeLog const scope2(log, GLib::Flog::Level::Info, "Scoop2", "--");
		log.Info("s2");
		{
			GLib::Flog::ScopeLog const scope3(log, GLib::Flog::Level::Info, "Scoop3", "++");
			log.Info("s3");
			static_cast<void>(scope3);
		}
		static_cast<void>(scope2);
	}
	log.Info("End");

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string const contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());

	static_cast<void>(scope1);

	TEST(contents.find("] : INFO     : FlogTests::Fred  : Start") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : ==> Scoop1") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : s1") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  :  --> Scoop2") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : s2") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  :   ++> Scoop3") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : s3") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  :   <++ Scoop3") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  :  <-- Scoop2") != std::string::npos);
	TEST(contents.find("] : INFO     : FlogTests::Fred  : End") != std::string::npos);
}

AUTO_TEST_CASE(TestInterlevedLogs)
{
	auto const log1 = GLib::Flog::LogManager::GetLog("Jim");
	auto const log2 = GLib::Flog::LogManager::GetLog("Sheila");
	log1.Info("1");
	log2.Info("2");

	std::ifstream in(GLib::Flog::LogManager::GetLogPath());
	std::string const contents((std::istreambuf_iterator(in)), std::istreambuf_iterator<char>());

	TEST(contents.find("] : INFO     : Jim              : 1") != std::string::npos);
	TEST(contents.find("] : INFO     : Sheila           : 2") != std::string::npos);
}

AUTO_TEST_CASE(TestPendingScopeOverPrefixChange)
{
	auto const log1 = GLib::Flog::LogManager::GetLog("Jim");
	auto const log2 = GLib::Flog::LogManager::GetLog("Sheila");
	GLib::Flog::ScopeLog const scope1(log1, GLib::Flog::Level::Info, "Scoop1");
	{
		GLib::Flog::ScopeLog const scope2(log2, GLib::Flog::Level::Info, "Scoop2");
		static_cast<void>(scope2);
	}
	static_cast<void>(scope1);
}

AUTO_TEST_SUITE_END()