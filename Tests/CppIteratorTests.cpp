#include <boost/test/unit_test.hpp>

#include "../Coverage/SymbolNameUtils.h"

#include "TestUtils.h"

#include "GLib/Compat.h"
#include "GLib/Cpp/HtmlGenerator.h"

#include <fstream>

namespace GLib::Cpp
{
	std::ostream & operator<<(std::ostream & stm, Fragment const & f)
	{
		return stm << "State: " << f.first << ", Value: \'" << f.second << '\'';
	}
}

using GLib::Cpp::Fragment;
using GLib::Cpp::Holder;
using GLib::Cpp::State;

void Parse(std::string_view const code, bool const showWhiteSpace = {})
{
	for (auto const & x : Holder {code, showWhiteSpace})
	{
#ifdef CPP_DIAGS
		std::cout << x.first << " : " << x.second << std::endl;
#else
		static_cast<void>(x);
#endif
	}
}

AUTO_TEST_SUITE(CppIteratorTests)

AUTO_TEST_CASE(Empty)
{
	Holder const code {R"()", false};

	std::vector<Fragment> expected {};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(Code0)
{
	Holder const code {"void", false};

	std::vector<Fragment> expected {{State::Code, "void"}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(Code1)
{
	Holder const code {R"(void foo)", true};

	std::vector<Fragment> expected {{State::Code, "void"}, {State::WhiteSpace, " "}, {State::Code, "foo"}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CodeNoWs)
{
	Holder const code {R"(void foo)", false};

	std::vector<Fragment> const expected {{State::Code, "void foo"}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CommentBlock)
{
	Holder const code {R"(/***/)", false};

	std::vector<Fragment> const expected {{State::CommentBlock, {"/***/"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CommentWhiteSpace)
{
	Holder const code {"/**/ \n;", true};

	std::vector<Fragment> const expected {{State::CommentBlock, {"/**/"}}, {State::WhiteSpace, {" "}}, {State::Code, {"\n;"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CommentLineContinue)
{
	Holder const code {R"(// hello\
continue
/* block */ /* another block */
)",
										 true};

	std::vector<Fragment> const expected {
		{State::CommentLine, {"// hello\\\ncontinue\n"}},
		{State::CommentBlock, {"/* block */"}},
		{State::WhiteSpace, {" "}},
		{State::CommentBlock, {"/* another block */"}},
		{State::WhiteSpace, {"\n"}},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CommentLineNotContinue)
{
	Holder const code {"// hello \\ not continuation", false};

	std::vector<Fragment> const expected {{State::CommentLine, {"// hello \\ not continuation"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CommentLineNotContinueEnd)
{
	Holder const code {"// hello not continuation \\/", false};

	std::vector<Fragment> const expected {{State::CommentLine, {"// hello not continuation \\/"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CommentStar)
{
	Holder const code {"/* * */", false};

	std::vector<Fragment> const expected {{State::CommentBlock, {"/* * */"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(NotCommentStart)
{
	Holder const code {"int foo=bar/baz;", true};

	std::vector<Fragment> const expected {{State::Code, {"int"}}, {State::WhiteSpace, {" "}}, {State::Code, {"foo=bar"}}, {State::Code, {"/baz;"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CommentFromStateCode)
{
	Holder const code {"bar//comment\n;", false};

	std::vector<Fragment> const expected {{State::Code, {"bar"}}, {State::CommentLine, {"//comment\n"}}, {State::Code, {";"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(String)
{
	Holder const code {R"(auto fred = "this is a string";)", true};

	std::vector<Fragment> const expected {
		{State::Code, {"auto"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"fred"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"="}},
		{State::WhiteSpace, {" "}},
		{State::String, {"\"this is a string\""}},
		{State::Code, {";"}},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(StringFromStateCode)
{
	Holder const code {R"(;"hello";)", false};

	std::vector<Fragment> const expected {{State::Code, {";"}}, {State::String, {R"("hello")"}}, {State::Code, {";"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(StringContinue)
{
	Holder const code {R"--("abc\
def")--",
										 false};

	std::vector<Fragment> const expected {{State::String, {R"--("abc\
def")--"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(StringNotContinue)
{
	Holder const code {R"--("\\abc\\")--", false};

	std::vector<Fragment> const expected {{State::String, {R"--("\\abc\\")--"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(StringWithQuotes)
{
	Holder const code {R"--("\"abc\"")--", false};

	std::vector<Fragment> const expected {{State::String, {R"--("\"abc\"")--"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(RawString)
{
	Holder const code {R"--(auto fred = R"(this is a raw string)";)--", true};

	std::vector<Fragment> const expected {
		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}}, {State::Code, {"fred"}}, {State::WhiteSpace, {" "}},
		{State::Code, {"="}},		 {State::WhiteSpace, {" "}}, {State::Code, {"R"}},		{State::RawString, {"\"(this is a raw string)\""}},
		{State::Code, {";"}},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(RawStringPrefix)
{
	Holder const code {R"--(auto fred = R"==(this is a raw string)==";)--", true};

	std::vector<Fragment> const expected {
		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}}, {State::Code, {"fred"}}, {State::WhiteSpace, {" "}},
		{State::Code, {"="}},		 {State::WhiteSpace, {" "}}, {State::Code, {"R"}},		{State::RawString, {R"--("==(this is a raw string)==")--"}},
		{State::Code, {";"}},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(RawStringIgnored)
{
	Holder const code {R"(R"--(hello)--)--")", false};

	std::vector<Fragment> const expected {{State::Code, "R"}, {State::RawString, "\"--(hello)--)--\""}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(RawStringPrefixTooLong)
{
	std::string_view constexpr code = R"(R"12345678901234567(content)12345678901234567")";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: '7' (0x37) at line: 1, state: RawStringPrefix");
}

AUTO_TEST_CASE(RawStringPrefixErrorSpace)
{
	std::string_view constexpr code = R"(R" (content) ")";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code, true), "Illegal character: ' ' (0x20) at line: 1, state: RawStringPrefix");
}

AUTO_TEST_CASE(RawStringPrefixErrorCloseParenthesis)
{
	std::string_view constexpr code = R"--(R")(content)(")--";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: ')' (0x29) at line: 1, state: RawStringPrefix");
}

AUTO_TEST_CASE(RawStringPrefixBackslash)
{
	std::string_view constexpr code = R"--(R"\(content)\")--";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: '\\' (0x5c) at line: 1, state: RawStringPrefix");
}

AUTO_TEST_CASE(RawStringNewLine)
{
	Holder const code {
		R"--(R"(1
2
3)")--",
		false};

	std::vector<Fragment> const expected {
		{State::Code, {"R"}},
		{State::RawString, {R"--("(1
2
3)")--"}},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(Main)
{
	Holder const code {
		R"--(#include <iostream>

int main() // main
{
	std::cout << "HelloWorld!" << std::endl;
	std::cout << R"(HelloWorld!)" << std::endl;
	return 0;
}
)--",
		true};

	std::vector<Fragment> const expected {
		{State::Directive, {"#include <iostream>"}},
		{State::WhiteSpace, {"\n\n"}},
		{State::Code, {"int"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"main()"}},
		{State::WhiteSpace, {" "}},
		{State::CommentLine, {"// main\n"}},
		{State::Code, {"{"}},
		{State::WhiteSpace, {"\n\t"}},

		{State::Code, {"std::cout"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"<<"}},
		{State::WhiteSpace, {" "}},
		{State::String, {R"("HelloWorld!")"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"<<"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"std::endl;"}},
		{State::WhiteSpace, {"\n\t"}},

		{State::Code, {"std::cout"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"<<"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"R"}},
		{State::RawString, {"\"(HelloWorld!)\""}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"<<"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"std::endl;"}},
		{State::WhiteSpace, {"\n\t"}},

		{State::Code, {"return"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"0;"}},
		{State::WhiteSpace, {"\n"}},
		{State::Code, {"}"}},
		{State::WhiteSpace, {"\n"}},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(SystemInclude)
{
	Holder const code {R"--(#include <experimental/filesystem>)--", false};

	std::vector<Fragment> const expected {{State::Directive, {"#include <experimental"}}, {State::Directive, {"/filesystem>"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CharacterLiteral)
{
	Holder const code {R"(auto char1='"';
auto char2='\"';
auto char3='\\';
)",
										 true};

	std::vector<Fragment> const expected {
		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}},	{State::Code, {"char1="}}, {State::CharacterLiteral, {R"('"')"}},
		{State::Code, {";"}},		 {State::WhiteSpace, {"\n"}},

		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}},	{State::Code, {"char2="}}, {State::CharacterLiteral, {R"('\"')"}},
		{State::Code, {";"}},		 {State::WhiteSpace, {"\n"}},

		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}},	{State::Code, {"char3="}}, {State::CharacterLiteral, {R"('\\')"}},
		{State::Code, {";"}},		 {State::WhiteSpace, {"\n"}},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CharacterLiteralFromStateNone)
{
	Holder const code {R"('\x00';)", false};

	std::vector<Fragment> const expected {{State::CharacterLiteral, {R"('\x00')"}}, {State::Code, {";"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(CharacterLiteralFromStateWhitespace)
{
	Holder const code {R"( '\x00';)", true};

	std::vector<Fragment> const expected {{State::WhiteSpace, {" "}}, {State::CharacterLiteral, {R"('\x00')"}}, {State::Code, {";"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(NotCharacterLiteral)
{
	Holder const code {R"(0xFFFF'FFFFU;)", false};

	std::vector<Fragment> const expected {{State::Code, {"0xFFFF'FFFFU;"}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(Guard)
{
	Holder const code {R"(/* comment */
#ifndef file_included // another comment
#define file_included

#endif /* not file_included */
)",
										 true};

	std::vector<Fragment> const expected {
		{State::CommentBlock, "/* comment */"},
		{State::WhiteSpace, "\n"},
		{State::Directive, "#ifndef file_included "},
		{State::CommentLine, "// another comment\n"},
		{State::Directive, "#define file_included"},
		{State::WhiteSpace, "\n\n"},
		{State::Directive, "#endif "},
		{State::CommentBlock, "/* not file_included */"},
		{State::WhiteSpace, "\n"},
	};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(DirectiveContinue)
{
	Holder const code {R"(#include \
"foo")",
										 false};

	std::vector<Fragment> const expected {{State::Directive, "#include \\\n\"foo\""}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(TerminationError)
{
	std::string_view constexpr code = R"("stringNotClosed)";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Termination error, State: String, StartLine: 1");
}

AUTO_TEST_CASE(DirectiveNotContinue)
{
	auto const code = Holder {R"(# define foo \ //)", false};

	std::vector<Fragment> const expected {{State::Directive, "# define foo \\ "}, {State::CommentLine, "//"}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

AUTO_TEST_CASE(Html)
{
	std::string_view constexpr code = ";";

	std::ostringstream stm;
	Htmlify(code, true, stm);

	auto const * const expected = ";";
	TEST(expected == stm.str());
}

AUTO_TEST_CASE(Html2)
{
	std::string_view constexpr code = "#include \"foo.h\"";

	std::ostringstream stm;
	Htmlify(code, true, stm);

	auto const * const expected = "<span class=\"d\">#include\xC2\xB7&quot;foo.h&quot;</span>";

	TEST(expected == stm.str());
}

AUTO_TEST_CASE(Html3)
{
	std::string_view constexpr code = R"(/*
1
2
3
*/)";

	std::ostringstream stm;
	Htmlify(code, true, stm);

	auto const * expected = R"(<span class="c">/*</span>
<span class="c">1</span>
<span class="c">2</span>
<span class="c">3</span>
<span class="c">*/</span>)";

	TEST(expected == stm.str());
}

AUTO_TEST_CASE(KeywordAndCommonType)
{
	std::string_view constexpr code = "auto v=std::vector{};";

	std::ostringstream stm;
	Htmlify(code, true, stm);

	auto const * expected = "<span class=\"k\">auto</span>"
													"<span class=\"w\">\xC2\xB7</span>"
													"v="
													"std::<span class=\"t\">vector</span>"
													"{};";

	TestUtils::Compare(stm.str(), expected);
}

AUTO_TEST_CASE(SymbolNameCleanup)
{
	std::string value = "NoCleanUp";
	RemoveTemplateDefinitions(value);
	TEST("NoCleanUp" == value);

	value = "Foo<T1,T2>::Bar<T3>";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T>::Bar<T>" == value);

	value = "Foo<Bar, Baz>::Qux<Quux, Quuz>";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T>::Qux<T>" == value);

	value = "Foo<Bar, Baz<Qux<Quux, Quuz>>>::Corge";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T>::Corge" == value);
}

AUTO_TEST_CASE(SymbolNamePreOps)
{
	std::string value = "Foo<Bar>::operator->";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T>::operator->" == value);

	value = "operator> Foo<Bar>";
	RemoveTemplateDefinitions(value);
	TEST("operator> Foo<T>" == value);

	value = "operator>> Foo<Bar>";
	RemoveTemplateDefinitions(value);
	TEST("operator>> Foo<T>" == value);

	value = "operator< Foo<Bar>";
	RemoveTemplateDefinitions(value);
	TEST("operator< Foo<T>" == value);

	value = "operator<< Foo<Bar>";
	RemoveTemplateDefinitions(value);
	TEST("operator<< Foo<T>" == value);
}

AUTO_TEST_CASE(SymbolNamePostOps)
{
	std::string value = "Foo<Bar> operator>";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T> operator>" == value);

	value = "Foo<Bar> operator>>";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T> operator>>" == value);

	value = "Foo<Bar> operator<";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T> operator<" == value);

	value = "Foo<Bar> operator<<";
	RemoveTemplateDefinitions(value);
	TEST("Foo<T> operator<<" == value);
}

AUTO_TEST_CASE(SymbolNameError)
{
	std::string value = ">foo<";
	GLIB_CHECK_RUNTIME_EXCEPTION({ RemoveTemplateDefinitions(value); }, "Unable to parse symbol: >foo<");
}

AUTO_TEST_CASE(UnterminatedBug)
{
	std::string_view constexpr code = R"(//\)"; // test compilers have no error
	std::ostringstream stm;
	GLIB_CHECK_RUNTIME_EXCEPTION({ Htmlify(code, false, stm); }, "Termination error, State: CommentLine, StartLine: 1");
}

// #define BULK_TEST
#ifdef BULK_TEST
void ScanFile(std::filesystem::path const & p, std::ostream & stm)
{
	std::ifstream t(p);
	if (!t)
	{
		std::cout << "read failed : " << p << '\n';
		return;
	}

	try
	{
		std::stringstream ss;
		ss << t.rdbuf();
		Parse(ss.str(), true);
	}
	catch (std::runtime_error const & e)
	{
		stm << p << " : " << e.what() << '\n';
	}
}

AUTO_TEST_CASE(BulkTest)
{
	auto source = std::filesystem::path {__FILE__}.parent_path().parent_path();
	std::unordered_set<std::string_view> extensions {".c", ".cpp", ".h", ".hpp"};

	std::list<std::filesystem::path> paths = {
#if defined(__linux__) && defined(__GNUG__)
		"/usr/include"
#elif defined(_WIN32) && defined(_MSC_VER)
		source / "Coverage",
		source / "GLib",
		source / "include",
		source / "Tests",

		// needs getting the tools/version
		R"--(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.31.31103\include)--",
		R"--(C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.31.31103\crt)--",
		R"--(C:\Users\Glen\source\ExternalDependencies\boost_1_78_0_test\boost)--"
#else
#error unknown
#endif
	};

	std::ostringstream stm;
	for (auto const & p : paths)
	{
		size_t count {};
		for (auto const & de : std::filesystem::recursive_directory_iterator(p))
		{
			if (is_regular_file(de.path()) && extensions.contains(de.path().extension().string()))
			{
				ScanFile(de.path(), stm);
				++count;
			}
		}
		stm << "BulkTest: " << p << ", Count: " << count << '\n';
	}

	auto state = boost::unit_test::unit_test_log.set_threshold_level(boost::unit_test::log_level::log_messages);
	static_cast<void>(state);
	TEST_MESSAGE(stm.str());
	// static_cast<void>(boost::unit_test::unit_test_log.set_threshold_level(state)); // does no restore
	static_cast<void>(boost::unit_test::unit_test_log.set_threshold_level(boost::unit_test::log_level::log_nothing));
}
#endif // BULK_TEST

AUTO_TEST_SUITE_END()
