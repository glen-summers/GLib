
#ifdef _DEBUG
#define COM_PTR_DEBUG
#endif

#include <Windows.h>

#include <GLib/Win/Automation.h>
#include <GLib/Win/ComUtils.h>
#include <GLib/Win/DebugStream.h>
#include <GLib/Win/DebugWrite.h>
#include <GLib/Win/FileSystem.h>
#include <GLib/Win/NativeExceptionPrinter.h>
#include <GLib/Win/Process.h>
#include <GLib/Win/Registry.h>
#include <GLib/Win/Symbols.h>
#include <GLib/Win/Uuid.h>
#include <GLib/Win/Variant.h>
#include <GLib/Win/Window.h>
#include <GLib/Win/WindowFinder.h>

#include <boost/test/unit_test.hpp>

#include "TestUtils.h"

using namespace std::chrono_literals;

using GLib::Cvt::A2W;
using GLib::Cvt::P2A;

using namespace GLib::Win; // NOLINT(google-build-using-namespace)

namespace boost::test_tools::tt_detail
{
	template <>
	struct print_log_value<Variant>
	{
		inline void operator()(std::ostream & str, const Variant & item)
		{
			str << item.Type();
		}
	};
}

namespace
{
	LONG WINAPI Filter(std::ostream & s, EXCEPTION_POINTERS * exceptionInfo)
	{
		Symbols::Print(s, exceptionInfo, I32(100));
		return EXCEPTION_EXECUTE_HANDLER;
	}

	template <typename Function>
	void GetStackTrace(std::ostream & s, const Function & function)
	{
		__try // NOLINT(clang-diagnostic-language-extension-token)
		{
			function();
		}
		__except (Filter(s, GetExceptionInformation())) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
		{}
	}

	template <typename Function>
	std::string GetStackTrace(const Function & function)
	{
		std::ostringstream s;
		GetStackTrace(s, function);
		// std::cout << s.str();
		return s.str();
	}

	std::filesystem::path GetTestApp()
	{
		std::string appName = "TestApp.exe";
		std::string cmakeName = "TestApp/" + appName;
		auto p = std::filesystem::path(Process::CurrentPath()).parent_path();
		if (exists(p / appName))
		{
			p /= appName;
		}
		else if (exists(p.parent_path() / cmakeName))
		{
			p = p.parent_path() / cmakeName;
		}
		else
		{
			throw std::runtime_error(appName + " not found");
		}
		return p;
	}

	const auto DefaultTimeout = 10s;
}

// split up
AUTO_TEST_SUITE(WinTests)

AUTO_TEST_CASE(TestDriveInfo)
{
	auto ld = FileSystem::LogicalDrives();
	static_cast<void>(ld);
	auto dm = FileSystem::DriveMap();

	std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / A2W(ToString(Util::Uuid::CreateRandom()) + ".tmp");
	Handle h(FileSystem::CreateAutoDeleteFile(P2A(tempFilePath)));
	std::string pathOfHandle = FileSystem::PathOfFileHandle(h.get(), VOLUME_NAME_NT);

	TEST(!exists(std::filesystem::path(pathOfHandle)));
	auto normalisedPath = FileSystem::NormalisePath(pathOfHandle, dm);
	TEST(exists(std::filesystem::path(normalisedPath)));
}

AUTO_TEST_CASE(TestPathOfModule)
{
	std::string path = FileSystem::PathOfModule(Process::CurrentModule());
	TEST("Tests.exe" == P2A(std::filesystem::path(path).filename()));
}

AUTO_TEST_CASE(TestPathOfhandle)
{
	std::filesystem::path tempFilePath = std::filesystem::temp_directory_path() / A2W(ToString(Util::Uuid::CreateRandom()) + "\xE2\x82\xAC.tmp");

	Handle h(FileSystem::CreateAutoDeleteFile(P2A(tempFilePath)));
	std::wstring pathOfHandle = A2W(FileSystem::PathOfFileHandle(h.get(), 0));
	TEST(true == exists(std::filesystem::path(pathOfHandle)));
	h.reset();
	TEST(false == exists(std::filesystem::path(pathOfHandle)));
}

AUTO_TEST_CASE(TestDebugStream)
{
	// capture output
	Debug::Stream() << "Hello!" << std::endl;
	Debug::Write("DebugStreamTest1");
	Debug::Write("DebugStreamTest2 {0} {1} {2}", 1, 2, 3);
	Debug::Write("Utf8 \xE2\x82\xAC");
	OutputDebugStringA("Write utf8 \xE2\x82\xAC directly for Debugger test\r\n");
}

AUTO_TEST_CASE(TestProcess)
{
	// capture output
	Process p(R"(c:\windows\system32\cmd.exe)"); //  /c echo hello
	{
		auto scopedTerminator = p.ScopedTerminator();
		TEST(p.IsRunning());
		static_cast<void>(scopedTerminator);
	}

	TEST(!p.IsRunning());
	TEST(1U == p.ExitCode());
}

AUTO_TEST_CASE(TestErrorCheck)
{
	SetLastError(ERROR_ACCESS_DENIED);
	GLIB_CHECK_EXCEPTION(Util::AssertTrue(false, "test fail"), WinException, "test fail : Access is denied. (5)");
}

AUTO_TEST_CASE(RegistryKeyExists)
{
	TEST(RegistryKeys::LocalMachine.KeyExists("Software"));
	TEST(!RegistryKeys::LocalMachine.KeyExists("MissingFooBar"));
}

AUTO_TEST_CASE(RegistryGetString)
{
	auto key = RegistryKeys::CurrentUser / "Environment";
	std::string regValue = key.GetString("TEMP");

	std::string envValue = *GLib::Compat::GetEnv("TEMP");
	envValue = FileSystem::LongPath(envValue);
	TEST(envValue == regValue);
}

AUTO_TEST_CASE(RegistryMissingValue)
{
	GLIB_CHECK_EXCEPTION(static_cast<void>(RegistryKeys::LocalMachine.GetString("MissingFooBar")), WinException,
											 "RegGetValue : The system cannot find the file specified. (2)");
}

AUTO_TEST_CASE(RegistryGet)
{
	auto regValue = RegistryKeys::CurrentUser / "Environment" & "TEMP";

	std::string envValue = *GLib::Compat::GetEnv("TEMP");
	envValue = FileSystem::LongPath(envValue);

	TEST(std::holds_alternative<std::string>(regValue));
	TEST(envValue == std::get<std::string>(regValue));
}

AUTO_TEST_CASE(RegistryTestComItf)
{
	std::string keyPath = GLib::Formatter::Format("Interface\\{0}", Util::Uuid {IID_IUnknown});
	TEST(RegistryKeys::ClassesRoot.KeyExists(keyPath));
	TEST(RegistryKeys::ClassesRoot.OpenSubKey(keyPath).GetString("") == "IUnknown");
}

AUTO_TEST_CASE(RegistryCreateKey)
{
	const auto & rootKey = RegistryKeys::CurrentUser;
	constexpr std::string_view testKey {"Software\\CrapolaCppUnitTests"};

	auto key = rootKey.CreateSubKey(testKey);

	auto scopedDelete = GLib::Detail::Scope([&]() { static_cast<void>(rootKey.DeleteSubKey(testKey)); });

	TEST(rootKey.KeyExists(testKey));
	key.SetInt32("Int32Value", I32(1234567890));
	key.SetInt64("Int64Value", I64(12345678901234567890));
	key.SetString("StringValue", "plugh");

	TEST(1234567890U == key.GetInt32("Int32Value"));
	TEST(12345678901234567890U == key.GetInt64("Int64Value"));
	TEST(1234567890U == key.GetInt64("Int32Value"));
	TEST("plugh" == key.GetString("StringValue"));

	GLIB_CHECK_RUNTIME_EXCEPTION(static_cast<void>(key.GetInt32("Int64Value")), "RegQueryValueEx : More data is available. (234)");
	GLIB_CHECK_RUNTIME_EXCEPTION(static_cast<void>(key.GetInt32("StringValue")), "RegQueryValueEx : More data is available. (234)");
	GLIB_CHECK_RUNTIME_EXCEPTION(static_cast<void>(key.GetInt64("StringValue")), "RegQueryValueEx : More data is available. (234)");
	GLIB_CHECK_RUNTIME_EXCEPTION(static_cast<void>(key.GetString("Int32Value")), "RegGetValue : Data of this type is not supported. (1630)");
	GLIB_CHECK_RUNTIME_EXCEPTION(static_cast<void>(key.GetString("Int64Value")), "RegGetValue : Data of this type is not supported. (1630)");

	TEST(1234567890U == std::get<uint32_t>(key.Get("Int32Value")));
	TEST(12345678901234567890U == std::get<uint64_t>(key.Get("Int64Value")));
	TEST("plugh" == std::get<std::string>(key.Get("StringValue")));

	static_cast<void>(rootKey.DeleteSubKey(testKey));
	TEST(!rootKey.KeyExists(testKey));

	static_cast<void>(scopedDelete);
}

AUTO_TEST_CASE(PrintNativeException1)
{
	auto s = GetStackTrace([]() { throw std::runtime_error("!"); });

	TEST(s.find("Unhandled exception at") != std::string::npos);
	TEST(s.find("(code: E06D7363) : C++ exception of type: 'class std::runtime_error'") != std::string::npos);
	TEST(s.find("RaiseException") != std::string::npos);
	TEST(s.find("CxxThrowException") != std::string::npos);
	TEST(s.find("GetStackTrace") != std::string::npos);
	TEST(s.find("WinTests::PrintNativeException1") != std::string::npos);
}

AUTO_TEST_CASE(PrintNativeException2)
{
	auto s = GetStackTrace([]() { throw I32(12345678); }); // NOLINT(hicpp-exception-baseclass)

	TEST(s.find("Unhandled exception at") != std::string::npos);
	TEST(s.find("(code: E06D7363) : C++ exception of type: 'int'") != std::string::npos);
	TEST(s.find("RaiseException") != std::string::npos);
	TEST(s.find("CxxThrowException") != std::string::npos);
	TEST(s.find("GetStackTrace") != std::string::npos);
	TEST(s.find("WinTests::PrintNativeException2") != std::string::npos);
}

AUTO_TEST_CASE(PrintNativeException3)
{
	const int * p = nullptr;
	int result {};
	auto s = GetStackTrace([&]() { result = *p++; }); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

	TEST(s.find("Unhandled exception at") != std::string::npos);
	TEST(s.find("(code: C0000005) : Access violation reading address 00000000") != std::string::npos);
	TEST(s.find("GetStackTrace") != std::string::npos);
	TEST(s.find("WinTests::PrintNativeException3") != std::string::npos);
}

AUTO_TEST_CASE(TestVariant)
{
	Variant v0;
	Variant v1 {"v"};
	Variant v2 {"v"};
	Variant v3 {"v3"};

	TEST(VT_EMPTY == v0.Type());
	TEST(VT_BSTR == v1.Type());
	TEST(v1 == v2);
	TEST(v1 != v3);
	TEST("v" == v1.ToString());
	TEST("v" == v2.ToString());
	TEST("v3" == v3.ToString());

	auto v4 = v1;
	TEST("v" == v4.ToString());
	v4 = v3;
	TEST("v3" == v4.ToString());

	auto v5 = std::move(v2);
	TEST("v" == v5.ToString());
	TEST(VT_EMPTY == v2.Type());

	v5 = std::move(v3);
	TEST("v3" == v5.ToString());
	TEST(VT_EMPTY == v3.Type());
}

AUTO_TEST_CASE(TestApp0)
{
	Process p(P2A(GetTestApp()), "-exitTime 1", 0, SW_HIDE);
	auto scopedTerminator(p.ScopedTerminator());
	p.WaitForExit(DefaultTimeout);
	static_cast<void>(scopedTerminator.release());
	CHECK(0UL == p.ExitCode());
}

AUTO_TEST_CASE(TestApp1)
{
	Mta mta;

	Automation aut; // causes leaks
	Process p(P2A(GetTestApp()), "-exitTime 500", 0, SW_SHOWNORMAL);
	auto scopedTerminator(p.ScopedTerminator());
	p.WaitForInputIdle(DefaultTimeout);

	HWND hw = WindowFinder::Find(p.Id(), "TestApp");
	TEST(hw != nullptr);
	Element mainWindow = aut.ElementFromHandle(hw);

	TEST("TestApp" == mainWindow.CurrentName());
	CHECK(mainWindow.CurrentClassName().find("GTL:") == 0);

	auto wp(mainWindow.GetCurrentPattern<IUIAutomationWindowPattern>(UIA_WindowPatternId));

	BOOL value {};
	CheckHr(wp->get_CurrentCanMaximize(&value), "get_CurrentCanMaximize");
	CHECK(value == TRUE);
	CheckHr(wp->SetWindowVisualState(WindowVisualState_Maximized), "SetWindowVisualState");

	CheckHr(wp->Close(), "Close");

	p.WaitForExit(DefaultTimeout);
	static_cast<void>(scopedTerminator.release());

	CHECK(0UL == p.ExitCode());
	static_cast<void>(mta);
}

AUTO_TEST_SUITE_END()