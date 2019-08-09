#include <boost/test/unit_test.hpp>

#include <Windows.h>

#include "GLib/Win/FileSystem.h"
#include "GLib/Win/DebugStream.h"
#include "GLib/Win/DebugWrite.h"
#include "GLib/Win/Process.h"
#include "GLib/Win/Symbols.h"
#include "GLib/Win/Uuid.h"
#include "GLib/XmlPrinter.h"

#include "TestUtils.h"


// split up
// clone hg tests in
BOOST_AUTO_TEST_SUITE(WinTests)

	BOOST_AUTO_TEST_CASE(TestDriveInfo)
	{
		auto ld = GLib::Win::FileSystem::LogicalDrives();
		(void)ld;
		auto dm = GLib::Win::FileSystem::DriveMap();

		std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / GLib::Cvt::a2w(to_string(GLib::Win::Util::Uuid::CreateRandom()) + ".tmp");
		GLib::Win::Handle h(GLib::Win::FileSystem::CreateAutoDeleteFile(tempFilePath.u8string()));
		std::string pathOfHandle = GLib::Win::FileSystem::PathOfFileHandle(h.get(), VOLUME_NAME_NT);

		BOOST_TEST(!exists(std::filesystem::u8path(pathOfHandle)));
		auto normalisedPath = GLib::Win::FileSystem::NormalisePath(pathOfHandle, dm);
		BOOST_TEST(exists(std::filesystem::u8path(normalisedPath)));
	}

	BOOST_AUTO_TEST_CASE(TestPathOfModule)
	{
		std::string path = GLib::Win::FileSystem::PathOfModule(GLib::Win::Process::CurrentModule());
		BOOST_TEST("Tests.exe" == std::filesystem::path(path).filename().u8string());
	}

	BOOST_AUTO_TEST_CASE(TestPathOfhandle)
	{
		std::filesystem::path tempFilePath = std::filesystem::temp_directory_path()
			/ GLib::Cvt::a2w(to_string(GLib::Win::Util::Uuid::CreateRandom()) + "\xE2\x82\xAC.tmp");

		GLib::Win::Handle h(GLib::Win::FileSystem::CreateAutoDeleteFile(tempFilePath.u8string()));
		std::string pathOfHandle = GLib::Win::FileSystem::PathOfFileHandle(h.get(), 0);
		BOOST_TEST(true == exists(std::filesystem::u8path(pathOfHandle)));
		h.reset();
		BOOST_TEST(false == exists(std::filesystem::u8path(pathOfHandle)));
	}

	BOOST_AUTO_TEST_CASE(TestDebugStream)
	{
		// capture output
		GLib::Win::Debug::Stream() << "Hello!" << std::endl;
		GLib::Win::Debug::Write("DebugStreamTest1");
		GLib::Win::Debug::Write("DebugStreamTest2 {0} {1} {2}", 1, 2, 3);
		GLib::Win::Debug::Write("Utf8 \xE2\x82\xAC");
		::OutputDebugStringA("Write utf8 \xE2\x82\xAC directly for Debugger test\r\n");
	}

	BOOST_AUTO_TEST_CASE(TestProcess)
	{
		// capture output
		GLib::Win::Process p(R"(c:\windows\system32\cmd.exe)"); //  /c echo hello
		{
			auto scopedTerminator = p.ScopedTerminator();
			BOOST_TEST(p.IsRunning());
			(void)scopedTerminator;
		}

		BOOST_TEST(!p.IsRunning());
		BOOST_TEST(1U == p.ExitCode());
	}

	BOOST_AUTO_TEST_CASE(TestErrorCheck)
	{
		::SetLastError(ERROR_ACCESS_DENIED);
		BOOST_CHECK_EXCEPTION_EX(GLib::Win::Util::AssertTrue(false, "test fail"),
			GLib::Win::WinException, TestUtils::ExpectException, "test fail : Access is denied. (5)");
	}

	BOOST_AUTO_TEST_CASE(EnvironmentVariable)
	{
		const size_t UNLEN = 256;
		wchar_t userName[UNLEN+1];
		DWORD userNameLen = 256;
		GLib::Win::Util::AssertTrue(::GetUserNameW(userName, &userNameLen), "GetUserNameW");

		std::string result = GLib::Win::Detail::EnvironmentVariable("USERNAME");
		BOOST_CHECK(result == GLib::Cvt::w2a(userName));

		auto path = GLib::Win::Detail::EnvironmentVariable("PATH");
	}

	BOOST_AUTO_TEST_CASE(TestXmlprinterEscapes) // move, expand
	{
		Xml::Printer p;
		p.PushText("Start && End");
		BOOST_TEST("Start &amp;&amp; End" == p.Xml());
	}

	BOOST_AUTO_TEST_CASE(TestXmlPrinterFormat)
	{
		{
			Xml::Printer formatted{ true };
			formatted.OpenElement("Root");
			formatted.OpenElement("Nested");
			formatted.CloseElement();
			formatted.CloseElement();
			std::string xmlFormatted = formatted.Xml();
			BOOST_TEST(R"(<Root>
 <Nested/>
</Root>
)" == xmlFormatted);
		}

		{
			Xml::Printer unFormatted{ false };
			unFormatted.OpenElement("Root");
			unFormatted.OpenElement("Nested");
			unFormatted.CloseElement();
			unFormatted.CloseElement();
			auto xmlUnFormatted = unFormatted.Xml();
			BOOST_TEST("<Root><Nested/></Root>" == xmlUnFormatted);
		}

		{
			Xml::Printer unFormatted2{ true };
			unFormatted2.OpenElement("Root", false);
			unFormatted2.OpenElement("Nested", false);
			unFormatted2.CloseElement(false);
			unFormatted2.CloseElement(false);
			auto xmlUnFormatted = unFormatted2.Xml();
			BOOST_TEST("<Root><Nested/></Root>" == xmlUnFormatted);
		}
	}

BOOST_AUTO_TEST_SUITE_END()