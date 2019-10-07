# GLib
This is currently a holdall for a series of C++ utilities created over a number of years and jobs to try and augment writing applications/services/utilities on Windows. The goals were low/no dependencies, testability, readability, and moderisation, target is C++17. Utf8 is used thoughout and coverted to Utf16 at Windows Api boundaries as per http://utf8everywhere.org/.

## Utilities include:
* Logging: FLog, a basic file [f]logging library
* CheckedCast: bounds checked numeric value conversion
* A basic compatibility layer to allow compilation on non-Windows systems, currently just Linux/GCC.
* Utf8<->Utf16 conversion wrappers to convert around Windows api calls
* String split iterators
* Scope macro to invoke a lambda during scope exit
* StackOrHeap optimisation for two-shot win api calls, reserves stack but can allocate heap if the stack was insufficient
* Evaluator: add C++ values/containers to an in-memory data store and evaluate/iterate properties to strings/ostreams
* TemplateEngine: uses Evaluator to implement a Thymeleaf like html generator, used by C++ coverage html report
* XmlStateEngine and C++ iterator: Used by TemplateEngine
* Formatter until C++20. I wrote this before noticing there was a similar C++20 specification. This version uses printf format strings
* Basic span until C++20, primarily to avoid Clang tidy warnings from pointer arithmetic

## WindowsSpecific
* Windows handle smart pointers
* Process Utilities
* Windows Exceptions from Api/Com error codes
* ComPtr type
* Implement Com types with static TypeLists
* ComCast method to call QueryIterface for ComPtr's
* DebugStream: ostream based stream to write to OutputDebugStream
* SymbolEngine: Used by Code coverage application to generate code coverage
* Filesystem utilities: some handy windows file system calls returning std::filesystem::path types;
* Debugger, CodeCoverage, Html Coverage report tool: Produces output code similar to LCOV, also adds C++ syntax highlighting source code output

## Testing
Boost test is used, the project expects Boost to be present in a directory up the path from the repository in a directory [[(../)*]ExternalDependencies/boost_[Ver]_TEST].
One of my other github utility repositories ./BoostModularBuild can be used to download and install boost test and dependencies via command line into an upstream ./ExternalDependencies/ directory, or if not present it will be installed to a temp directory in the GLib repository. This mechanism is used to allow automatic download and install of dependencies without administrator privileges, and for a single hive of source code repositories.
TODO: add go deps command, to download BoostModularBuild and install boost test

## Build
The build system allows multiple mechanisms, It can compile and run build targets for a VisualStudio solution or a from a mirrored cmake project. The cmake project also compiles on Linux/GCC for a basic compatibility test. The Visual Studio and cmake projects include a search facility to search up the directory tree to locate the ExternalDependencies directory.
The Windows command-line build is from a go.cmd at root level which checks Visual studio requirements then forwards on to an custom MsBuild project
From a windows prompt, build and run all tests for Windows Visual studio target:

	>go

Build and run all tests for Windows/Visual studio, Windows CMake, and Linux/GCC cmake

	>go all

Build and run tests for Windows/Visual studio, generate coverage information and launch a Html coverage report via the build in GLib coverage tool.

	>go coverage

Build and run tests for Linux build, generate coverage information and launches a Html coverage report via LCOV.

	>go wslcoverage

Delete all temporary files

	>go clean

## Linux Build
	$./go.sh {build|coverage|clean}

## Code Examples
### Logging: FLog, a basic file [f]logging library
	struct Fred {};
	auto log = GLib::Flog::LogManager::GetLog<Fred>();
	log.Info("Hello");
	log.Info("Format: {0} , {1}", 1, 2);

#### Output:
	------------------------------------------------
	Opened      : 06 Oct 2019, 12:37:49 (+0100)
	OpenedUtc   : 2019-10-06 11:37:49Z
	ProcessName : (64 bit) Tests.exe
	FullPath    : C:\Users\Glen\source\repos\github\GLib\msvc\x64\Debug\Tests.exe
	ProcessId   : 38840
	ThreadId    : 18056
	------------------------------------------------
	06 Oct 2019, 12:37:49.833 : [ 18056 ] : INFO     : FlogTests::Fred  : Hello
	06 Oct 2019, 12:37:49.836 : [ 18056 ] : INFO     : FlogTests::Fred  : Format: 1 , 2.
	------------------------------------------------
	Closed       06 Oct 2019, 12:37:49 (+0100)
	------------------------------------------------

### CheckedCast
	BOOST_TEST(1234u == checked_cast<unsigned int>(static_cast<short>(1234)));
	BOOST_TEST(static_cast<unsigned int>(std::numeric_limits<short>::max()) == checked_cast<unsigned int>(std::numeric_limits<short>::max()));
	BOOST_CHECK_EXCEPTION(checked_cast<unsigned int>(std::numeric_limits<short>::min()), Exception, IsUnderflow);

### Utf8<->Utf16 conversion wrappers
	std::string utf8Value = "\xE2\x82\xAC";
	std::wstring utf16Value = L"\u20AC";

	BOOST_TEST(utf16Value == GLib::Cvt::a2w(utf8Value));
	BOOST_TEST(utf8Value == GLib::Cvt::w2a(utf16Value));

### String split
	GLib::Util::SplitterView s("a<->bc<->def<->ghijkl", "<->");
	std::vector<std::string_view> expected { "a", "bc", "def", "ghijkl" };
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), s.begin(), s.end());

### Scope macro
	BSTR description = nullptr;
	pErrorInfo->GetDescription(&description);
	SCOPE(_, [=] () noexcept
	{
		::SysFreeString(description);
		pErrorInfo->Release();
	});

### StackOrHeap
	inline std::string PathOfFileHandle(HANDLE fileHandle, DWORD flags)
	{
		Util::StackOrHeap<wchar_t, DefaultStackReserveSize>
		DWORD length = ::GetFinalPathNameByHandleW(fileHandle, nullptr, 0, flags);
		Util::AssertTrue(length != 0, "GetFinalPathNameByHandleW failed");
		s.EnsureSize(length);
		length = ::GetFinalPathNameByHandleW(fileHandle, s.Get(), static_cast<DWORD>(s.size()), flags);
		Util::AssertTrue(length != 0 && length < s.size(), "GetFinalPathNameByHandleW failed");
		return Cvt::w2a(std::wstring_view{s.Get(), length});
	}

### Evaluator
	struct User
	{
		std::string name;
		int age;
		std::list<std::string> hobbies;
	};

	template <>
	struct GLib::Eval::Visitor<User>
	{
		static void Visit(const User & user, const std::string & propertyName, const ValueVisitor & f)
		{
			if (propertyName == "name")
			{
				return f(Value(user.name));
			}
			if (propertyName == "age")
			{
				return f(Value(user.age));
			}
			if (propertyName == "hobbies")
			{
				return f(MakeCollection(user.hobbies));
			}
			throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
		}
	};

	BOOST_AUTO_TEST_CASE(AddStruct)
	{
		GLib::Eval::Evaluator evaluator;
		User user {"Zardoz", 999, {}};
		evaluator.Set("user", user);

		std::string name = evaluator.Evaluate("user.name");
		BOOST_TEST(name == "Zardoz");
		std::string age = evaluator.Evaluate("user.age");
		BOOST_TEST(age == "999");
	}

### TemplateEngine
	BOOST_AUTO_TEST_CASE(ForEach)
	{
		const std::vector<User> users
		{
			{ "Fred", 42, { "FC00"} }, { "Jim", 43, { "FD00"} }, { "Sheila", 44, { "FE00"} }
		};
		Evaluator evaluator;
		evaluator.SetCollection("users", users);

		auto xml = R"(<xml xmlns:gl='glib'>
	<gl:block each="user : ${users}">
		<User name='${user.name}' />
	</gl:block>
	</xml>)";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

		auto expected= R"(<xml>
		<User name='Fred' />
		<User name='Jim' />
		<User name='Sheila' />
	</xml>)";

		BOOST_TEST(stm.str() == expected);
	}

### XmlStateEngine and iterator
	BOOST_AUTO_TEST_CASE(Attributes)
	{
		Xml::Holder xml { R"(<root a='1' b='2' >
		<sub c='3' d="4"/>
	</root>)" };

		std::vector<Xml::Element> expected
		{
			{"root", Xml::ElementType::Open, {"a='1' b='2'" }},
			{"sub", Xml::ElementType::Empty, {"c='3' d=\"4\""}},
			{"root", Xml::ElementType::Close, {}},
		};
		BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
	}

### Formatter
	BOOST_AUTO_TEST_CASE(TestRepeatedInsert)
	{
		std::string s = Formatter::Format("{0} {0:%x}", 1234);
		BOOST_TEST("1234 4d2" == s);
	}

	BOOST_AUTO_TEST_CASE(TestSprintfFormatPassThrough)
	{
		std::string s = Formatter::Format("{0:%#.8X}", 1234);
		BOOST_TEST("0X000004D2" ==s);
	}

### Windows handle smart pointers / Process Utilities
	BOOST_AUTO_TEST_CASE(TestProcess)
	{
		GLib::Win::Process p(R"(c:\windows\system32\cmd.exe)");
		{
			auto scopedTerminator = p.ScopedTerminator();
			BOOST_TEST(p.IsRunning());
			(void)scopedTerminator;
		}

		BOOST_TEST(!p.IsRunning());
		BOOST_TEST(1U == p.ExitCode());
	}

### Windows Exceptions from Api/Com error codes
	BOOST_AUTO_TEST_CASE(TestErrorCheck)
	{
		::SetLastError(ERROR_ACCESS_DENIED);
		GLIB_CHECK_EXCEPTION(GLib::Win::Util::AssertTrue(false, "test fail"), GLib::Win::WinException, "test fail : Access is denied. (5)");
	}

### ComPtr type / ComCast method to call QueryIterface for ComPtr's
	BOOST_AUTO_TEST_CASE(TestComErrorCheckWithErrorInfo)
	{
		GLib::Win::ComPtr<ICreateErrorInfo> p;
		GLib::Win::CheckHr(::CreateErrorInfo(&p), "CreateErrorInfo");
		auto ei = GLib::Win::ComCast<IErrorInfo>(p);
		p->SetDescription(const_cast<LPOLESTR>(L"hello"));
		GLib::Win::CheckHr(::SetErrorInfo(0, ei.Get()), "SetErrorInfo");

		GLIB_CHECK_EXCEPTION(GLib::Win::CheckHr(E_OUTOFMEMORY, "fail"),
		GLib::Win::ComException, "fail : hello (8007000E)");
	}

### Implement Com types with static TypeLists
	class ImplementsITest1AndITest2 final : public GLib::Win::Unknown<ITest1, ITest2>, public DeleteCounter<ImplementsITest1AndITest2>
	{
		GLIB_COM_RULE_OF_FIVE(ImplementsITest1AndITest2)

		HRESULT STDMETHODCALLTYPE Test1Method() override
		{
			return S_OK;
		}

		HRESULT STDMETHODCALLTYPE Test2Method() override
		{
			return S_OK;
		}
	};

	BOOST_AUTO_TEST_CASE(TestQIOk)
	{
		GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1AndITest2>());
		GLib::Win::ComPtr<ITest1> p2;
		HRESULT hr = p1->QueryInterface(&p2);
		BOOST_TEST(S_OK == hr);
		BOOST_TEST(2U == p1.UseCount());
		BOOST_TEST(2U == p2.UseCount());

		GLib::Win::ComPtr<IUnknown> pu;
		hr = p2->QueryInterface(&pu);
		BOOST_TEST(S_OK == hr);
		BOOST_TEST(pu.Get() == p2.Get());
	}

### DebugStream: ostream based stream to write to OutputDebugStream
	BOOST_AUTO_TEST_CASE(TestDebugStream)
	{
		GLib::Win::Debug::Stream() << "Hello!" << std::endl;
		GLib::Win::Debug::Write("DebugStreamTest1");
		GLib::Win::Debug::Write("DebugStreamTest2 {0} {1} {2}", 1, 2, 3);
		GLib::Win::Debug::Write("Utf8 \xE2\x82\xAC");
	}

### SymbolEngine
	const GLib::Win::Symbols::SymProcess & process = Symbols().GetProcess(processId);
	Symbols().Lines([&](PSRCCODEINFOW lineInfo)
	{
		AddLine(static_cast<const wchar_t *>(lineInfo->FileName), lineInfo->LineNumber, process, lineInfo->Address);
	}, process.Handle().get(), info.lpBaseOfImage);

### Filesystem utilities: some handy windows file system calls returning std::filesystem::path types
	inline std::vector<std::string> LogicalDrives();
	inline std::map<std::string, std::string> DriveMap();
	inline std::string PathOfFileHandle(HANDLE fileHandle, DWORD flags);
	inline std::string NormalisePath(const std::string & path, const std::map<std::string, std::string> & driveMap);
	inline std::string PathOfModule(HMODULE module);
	inline std::string PathOfProcessHandle(HANDLE process);
	inline Handle CreateAutoDeleteFile(const std::string & name);

### Debugger/Code Coverage/Html Coverage report tool: Produces windows output code similar to LCOV output, also adds C++ syntax highlighting source code
	>go coverage
