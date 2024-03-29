
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

template <>
struct boost::test_tools::tt_detail::print_log_value<Variant>
{
	void operator()(std::ostream & stm, Variant const & item) const
	{
		stm << item.Type();
	}
};

namespace
{
	LONG WINAPI Filter(std::ostream & stm, EXCEPTION_POINTERS const * exceptionInfo)
	{
		Symbols::Print(stm, exceptionInfo, I32(100));
		return EXCEPTION_EXECUTE_HANDLER;
	}

	template <typename Function>
	void GetStackTrace(std::ostream & stm, Function const & function)
	{
		__try // NOLINT(clang-diagnostic-language-extension-token)
		{
			function();
		}
		__except (Filter(stm, GetExceptionInformation())) // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
		{}
	}

	template <typename Function>
	std::string GetStackTrace(Function const & function)
	{
		std::ostringstream stm;
		GetStackTrace(stm, function);
		return stm.str();
	}

	std::filesystem::path GetTestApp()
	{
		std::string const appName = "TestApp.exe";
		std::string const cmakeName = "TestApp/" + appName;
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

	auto constexpr DefaultTimeout = 10s;
}

// split up
AUTO_TEST_SUITE(WinTests)

AUTO_TEST_CASE(TestDriveInfo)
{
	auto const ld = FileSystem::LogicalDrives();
	static_cast<void>(ld);
	auto const dm = FileSystem::DriveMap();

	std::filesystem::path const tempFilePath = std::filesystem::temp_directory_path() / A2W(ToString(Util::Uuid::CreateRandom()) + ".tmp");
	Handle const h(FileSystem::CreateAutoDeleteFile(P2A(tempFilePath)));
	std::string const pathOfHandle = FileSystem::PathOfFileHandle(h.get(), VOLUME_NAME_NT);

	TEST(!exists(std::filesystem::path(pathOfHandle)));
	auto const normalisedPath = FileSystem::NormalisePath(pathOfHandle, dm);
	TEST(exists(std::filesystem::path(normalisedPath)));
}

AUTO_TEST_CASE(TestPathOfModule)
{
	std::string const path = FileSystem::PathOfModule(Process::CurrentModule());
	TEST("Tests.exe" == P2A(std::filesystem::path(path).filename()));
}

AUTO_TEST_CASE(TestPathOfhandle)
{
	std::filesystem::path const tempFilePath = std::filesystem::temp_directory_path() / A2W(ToString(Util::Uuid::CreateRandom()) + "\xE2\x82\xAC.tmp");

	Handle h(FileSystem::CreateAutoDeleteFile(P2A(tempFilePath)));
	std::wstring const pathOfHandle = A2W(FileSystem::PathOfFileHandle(h.get(), 0));
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
	Process const p(R"(c:\windows\system32\cmd.exe)"); //  /c echo hello
	{
		auto const scopedTerminator = p.ScopedTerminator();
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
	auto const key = RegistryKeys::CurrentUser / "Environment";
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
	std::string const keyPath = GLib::Formatter::Format("Interface\\{0}", Util::Uuid {IID_IUnknown});
	TEST(RegistryKeys::ClassesRoot.KeyExists(keyPath));
	TEST(RegistryKeys::ClassesRoot.OpenSubKey(keyPath).GetString("") == "IUnknown");
}

AUTO_TEST_CASE(RegistryCreateKey)
{
	auto const & rootKey = RegistryKeys::CurrentUser;
	constexpr std::string_view testKey {"Software\\CrapolaCppUnitTests"};

	auto key = rootKey.CreateSubKey(testKey);

	auto const scopedDelete = GLib::Detail::Scope([&] { static_cast<void>(rootKey.DeleteSubKey(testKey)); });

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
	auto stm = GetStackTrace([] { throw std::runtime_error("!"); });

	TEST(stm.find("Unhandled exception at") != std::string::npos);
	TEST(stm.find("(code: E06D7363) : C++ exception of type: 'class std::runtime_error'") != std::string::npos);
	TEST(stm.find("RaiseException") != std::string::npos);
	TEST(stm.find("CxxThrowException") != std::string::npos);
	TEST(stm.find("GetStackTrace") != std::string::npos);
	TEST(stm.find("WinTests::PrintNativeException1") != std::string::npos);
}

AUTO_TEST_CASE(PrintNativeException2)
{
	auto stm = GetStackTrace([] { throw I32(12345678); }); // NOLINT(hicpp-exception-baseclass)

	TEST(stm.find("Unhandled exception at") != std::string::npos);
	TEST(stm.find("(code: E06D7363) : C++ exception of type: 'int'") != std::string::npos);
	TEST(stm.find("RaiseException") != std::string::npos);
	TEST(stm.find("CxxThrowException") != std::string::npos);
	TEST(stm.find("GetStackTrace") != std::string::npos);
	TEST(stm.find("WinTests::PrintNativeException2") != std::string::npos);
}

AUTO_TEST_CASE(PrintNativeException3)
{
	int const * p = nullptr;
	int result {};
	auto stm = GetStackTrace([&] { result = *p++; }); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	TEST(result == 0);
	TEST(stm.find("Unhandled exception at") != std::string::npos);
	TEST(stm.find("(code: C0000005) : Access violation reading address 00000000") != std::string::npos);
	TEST(stm.find("GetStackTrace") != std::string::npos);
	TEST(stm.find("WinTests::PrintNativeException3") != std::string::npos);
}

AUTO_TEST_CASE(TestVariant)
{
	Variant const v0;
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

	Variant v4 = v1;
	TEST("v" == v4.ToString());
	v4 = v3;
	TEST("v3" == v4.ToString());

	Variant v5 = std::move(v2);
	TEST("v" == v5.ToString());

#pragma warning(push)
#pragma warning(disable : 26800)
	TEST(VT_EMPTY == v2.Type());
#pragma warning(pop)

	v5 = std::move(v3);
	TEST("v3" == v5.ToString());

#pragma warning(push)
#pragma warning(disable : 26800)
	TEST(VT_EMPTY == v3.Type());
#pragma warning(pop)
}

AUTO_TEST_CASE(TestApp0)
{
	Process const p(P2A(GetTestApp()), "-exitTime 1", 0, SW_HIDE);
	auto scopedTerminator(p.ScopedTerminator());
	p.WaitForExit(DefaultTimeout);
	static_cast<void>(scopedTerminator.release());
	CHECK(0UL == p.ExitCode());
}

AUTO_TEST_CASE(TestApp1)
{
	Mta const mta;

	Automation const aut; // causes leaks
	Process const p(P2A(GetTestApp()), "-exitTime 500", 0, SW_SHOWNORMAL);
	auto scopedTerminator(p.ScopedTerminator());
	p.WaitForInputIdle(DefaultTimeout);

	HWND hw = WindowFinder::Find(p.Id(), "TestApp");
	TEST(hw != nullptr);
	Element mainWindow = aut.ElementFromHandle(hw);

	TEST("TestApp" == mainWindow.CurrentName());
	CHECK(mainWindow.CurrentClassName().starts_with("GTL:"));

	auto const wp(mainWindow.GetCurrentPattern<IUIAutomationWindowPattern>(UIA_WindowPatternId));

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