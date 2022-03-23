#include <boost/test/unit_test.hpp>

#include "../Coverage/SymbolNameUtils.h"

#include "TestUtils.h"

#include "GLib/Compat.h"
#include "GLib/Cpp/HtmlGenerator.h"

#include <fstream>

namespace GLib::Cpp
{
	std::ostream & operator<<(std::ostream & s, const Fragment & f)
	{
		return s << "State: " << f.first << ", Value: \'" << f.second << '\'';
	}
}

using namespace GLib::Cpp;

void Parse(const Holder & code)
{
	for (const auto & x : code)
	{
#ifdef CPP_DIAGS
		std::cout << x.first << " : " << x.second << std::endl;
#else
		static_cast<void>(x);
#endif
	}
}

BOOST_AUTO_TEST_SUITE(CppIteratorTests)

BOOST_AUTO_TEST_CASE(Empty)
{
	Holder code {R"()"};

	std::vector<Fragment> expected {};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Code0)
{
	Holder code {"void"};

	std::vector<Fragment> expected {{State::Code, "void"}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Code1)
{
	Holder code {R"(void foo)"};

	std::vector<Fragment> expected {{State::Code, "void"}, {State::WhiteSpace, " "}, {State::Code, "foo"}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CodeNoWs)
{
	Holder code {R"(void foo)", false};

	std::vector<Fragment> expected {{State::Code, "void foo"}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentBlock)
{
	Holder code {R"(/***/)"};

	std::vector<Fragment> expected {{State::CommentBlock, {"/***/"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentWhiteSpace)
{
	Holder code {R"(/**/ 
;)"};

	std::vector<Fragment> expected {{State::CommentBlock, {"/**/"}}, {State::WhiteSpace, {" "}}, {State::Code, {"\n;"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentLineContinue)
{
	Holder code {R"(// hello\
continue
/* block */ /* another block */
)"};

	std::vector<Fragment> expected {
		{State::CommentLine, {"// hello\\\ncontinue\n"}},
		{State::CommentBlock, {"/* block */"}},
		{State::WhiteSpace, {" "}},
		{State::CommentBlock, {"/* another block */"}},
		{State::WhiteSpace, {"\n"}},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentLineNotContinue)
{
	Holder code {"// hello \\ not continuation"};

	std::vector<Fragment> expected {{State::CommentLine, {"// hello \\ not continuation"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentLineNotContinueEnd)
{
	Holder code {"// hello not continuation \\/"};

	std::vector<Fragment> expected {{State::CommentLine, {"// hello not continuation \\/"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentStar)
{
	Holder code {"/* * */"};

	std::vector<Fragment> expected {{State::CommentBlock, {"/* * */"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(NotCommentStart)
{
	Holder code {"int foo=bar/baz;"};

	std::vector<Fragment> expected {{State::Code, {"int"}}, {State::WhiteSpace, {" "}}, {State::Code, {"foo=bar"}}, {State::Code, {"/baz;"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentFromStateCode)
{
	Holder code {"bar//comment\n;"};

	std::vector<Fragment> expected {{State::Code, {"bar"}}, {State::CommentLine, {"//comment\n"}}, {State::Code, {";"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(String)
{
	Holder code {R"(auto fred = "this is a string";)"};

	std::vector<Fragment> expected {
		{State::Code, {"auto"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"fred"}},
		{State::WhiteSpace, {" "}},
		{State::Code, {"="}},
		{State::WhiteSpace, {" "}},
		{State::String, {"\"this is a string\""}},
		{State::Code, {";"}},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(StringFromStateCode)
{
	Holder code {R"(;"hello";)"};

	std::vector<Fragment> expected {{State::Code, {";"}}, {State::String, {R"("hello")"}}, {State::Code, {";"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(StringContinue)
{
	Holder code {R"--("abc\
def")--"};

	std::vector<Fragment> expected {{State::String, {R"--("abc\
def")--"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(StringNotContinue)
{
	Holder code {R"--("\\abc\\")--"};

	std::vector<Fragment> expected {{State::String, {R"--("\\abc\\")--"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(StringWithQuotes)
{
	Holder code {R"--("\"abc\"")--"};

	std::vector<Fragment> expected {{State::String, {R"--("\"abc\"")--"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawString)
{
	Holder code {R"--(auto fred = R"(this is a raw string)";)--"};

	std::vector<Fragment> expected {
		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}}, {State::Code, {"fred"}}, {State::WhiteSpace, {" "}},
		{State::Code, {"="}},		 {State::WhiteSpace, {" "}}, {State::Code, {"R"}},		{State::RawString, {"\"(this is a raw string)\""}},
		{State::Code, {";"}},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawStringPrefix)
{
	Holder code {R"--(auto fred = R"==(this is a raw string)==";)--"};

	std::vector<Fragment> expected {
		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}}, {State::Code, {"fred"}}, {State::WhiteSpace, {" "}},
		{State::Code, {"="}},		 {State::WhiteSpace, {" "}}, {State::Code, {"R"}},		{State::RawString, {R"--("==(this is a raw string)==")--"}},
		{State::Code, {";"}},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawStringIgnored)
{
	Holder code {R"(R"--(hello)--)--")"};

	std::vector<Fragment> expected {{State::Code, "R"}, {State::RawString, "\"--(hello)--)--\""}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawStringPrefixTooLong)
{
	std::string_view code = R"(R"12345678901234567(content)12345678901234567")";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: '7' (0x37) at line: 1, state: RawStringPrefix");
}

BOOST_AUTO_TEST_CASE(RawStringPrefixErrorSpace)
{
	std::string_view code = R"(R" (content) ")";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: ' ' (0x20) at line: 1, state: RawStringPrefix");
}

BOOST_AUTO_TEST_CASE(RawStringPrefixErrorCloseParenthesis)
{
	std::string_view code = R"--(R")(content)(")--";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: ')' (0x29) at line: 1, state: RawStringPrefix");
}

BOOST_AUTO_TEST_CASE(RawStringPrefixBackslash)
{
	std::string_view code = R"--(R"\(content)\")--";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: '\\' (0x5c) at line: 1, state: RawStringPrefix");
}

BOOST_AUTO_TEST_CASE(RawStringNewLine)
{
	Holder code {
		R"--(R"(1
2
3)")--"};

	std::vector<Fragment> expected {
		{State::Code, {"R"}},
		{State::RawString, {R"--("(1
2
3)")--"}},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Main)
{
	Holder code {
		R"--(#include <iostream>

int main() // main
{
	std::cout << "HelloWorld!" << std::endl;
	std::cout << R"(HelloWorld!)" << std::endl;
	return 0;
}
)--"};

	std::vector<Fragment> expected {
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

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(SystemInclude)
{
	Holder code {R"--(#include <experimental/filesystem>)--"};

	std::vector<Fragment> expected {{State::Directive, {"#include <experimental"}}, {State::Directive, {"/filesystem>"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CharacterLiteral)
{
	Holder code {R"(auto char1='"';
auto char2='\"';
auto char3='\\';
)"};

	std::vector<Fragment> expected {
		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}},	{State::Code, {"char1="}}, {State::CharacterLiteral, {R"('"')"}},
		{State::Code, {";"}},		 {State::WhiteSpace, {"\n"}},

		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}},	{State::Code, {"char2="}}, {State::CharacterLiteral, {R"('\"')"}},
		{State::Code, {";"}},		 {State::WhiteSpace, {"\n"}},

		{State::Code, {"auto"}}, {State::WhiteSpace, {" "}},	{State::Code, {"char3="}}, {State::CharacterLiteral, {R"('\\')"}},
		{State::Code, {";"}},		 {State::WhiteSpace, {"\n"}},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CharacterLiteralFromStateNone)
{
	Holder code {R"('\x00';)"};

	std::vector<Fragment> expected {{State::CharacterLiteral, {R"('\x00')"}}, {State::Code, {";"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CharacterLiteralFromStateWhitespace)
{
	Holder code {R"( '\x00';)"};

	std::vector<Fragment> expected {{State::WhiteSpace, {" "}}, {State::CharacterLiteral, {R"('\x00')"}}, {State::Code, {";"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(NotCharacterLiteral)
{
	Holder code {R"(0xFFFF'FFFFU;)"};

	std::vector<Fragment> expected {{State::Code, {"0xFFFF'FFFFU;"}}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Guard)
{
	Holder code {R"(/* comment */
#ifndef file_included // another comment
#define file_included

#endif /* not file_included */
)"};

	std::vector<Fragment> expected {
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

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(DirectiveContinue)
{
	Holder code {R"(#include \
"foo")"};

	std::vector<Fragment> expected {{State::Directive, "#include \\\n\"foo\""}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(TerminationError)
{
	std::string_view code = R"("stringNotClosed)";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Termination error, State: String, StartLine: 1");
}

BOOST_AUTO_TEST_CASE(DirectiveNotContinue)
{
	auto code = Holder {R"(# define foo \ //)"};

	std::vector<Fragment> expected {{State::Directive, "# define foo \\ "}, {State::CommentLine, "//"}};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Html)
{
	std::string_view code = ";";

	std::ostringstream stm;
	Htmlify(code, stm);

	auto expected = ";";
	BOOST_TEST(expected == stm.str());
}

BOOST_AUTO_TEST_CASE(Html2)
{
	std::string_view code = "#include \"foo.h\"";

	std::ostringstream stm;
	Htmlify(code, stm);

	auto expected = "<span class=\"d\">#include\xC2\xB7&quot;foo.h&quot;</span>";

	BOOST_TEST(expected == stm.str());
}

BOOST_AUTO_TEST_CASE(Html3)
{
	std::string_view code = R"(/*
1
2
3
*/)";

	std::ostringstream stm;
	Htmlify(code, stm);

	auto expected = R"(<span class="c">/*</span>
<span class="c">1</span>
<span class="c">2</span>
<span class="c">3</span>
<span class="c">*/</span>)";

	BOOST_TEST(expected == stm.str());
}

BOOST_AUTO_TEST_CASE(KeywordAndCommonType)
{
	std::string_view code = "auto v=std::vector{};";

	std::ostringstream stm;
	Htmlify(code, stm);

	auto expected = "<span class=\"k\">auto</span>"
									"<span class=\"w\">\xC2\xB7</span>"
									"v="
									"std::<span class=\"t\">vector</span>"
									"{};";

	TestUtils::Compare(stm.str(), expected);
}

BOOST_AUTO_TEST_CASE(SymbolNameCleanup)
{
	std::string value = "NoCleanUp";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("NoCleanUp" == value);

	value = "Foo<T1,T2>::Bar<T3>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T>::Bar<T>" == value);

	value = "Foo<Bar, Baz>::Qux<Quux, Quuz>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T>::Qux<T>" == value);

	value = "Foo<Bar, Baz<Qux<Quux, Quuz>>>::Corge";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T>::Corge" == value);
}

BOOST_AUTO_TEST_CASE(SymbolNamePreOps)
{
	std::string value = "Foo<Bar>::operator->";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T>::operator->" == value);

	value = "operator> Foo<Bar>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("operator> Foo<T>" == value);

	value = "operator>> Foo<Bar>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("operator>> Foo<T>" == value);

	value = "operator< Foo<Bar>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("operator< Foo<T>" == value);

	value = "operator<< Foo<Bar>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("operator<< Foo<T>" == value);
}

BOOST_AUTO_TEST_CASE(SymbolNamePostOps)
{
	std::string value = "Foo<Bar> operator>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T> operator>" == value);

	value = "Foo<Bar> operator>>";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T> operator>>" == value);

	value = "Foo<Bar> operator<";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T> operator<" == value);

	value = "Foo<Bar> operator<<";
	RemoveTemplateDefinitions(value);
	BOOST_TEST("Foo<T> operator<<" == value);
}

BOOST_AUTO_TEST_CASE(SymbolNameError)
{
	std::string value = ">foo<";
	GLIB_CHECK_RUNTIME_EXCEPTION({ RemoveTemplateDefinitions(value); }, "Unable to parse symbol: >foo<");
}

BOOST_AUTO_TEST_CASE(UnterminatedBug)
{
	std::string_view code = R"(//\)"; // test compilers have no error
	std::ostringstream stm;
	GLIB_CHECK_RUNTIME_EXCEPTION({ Htmlify(code, stm); }, "Termination error, State: CommentLine, StartLine: 1");
}

//**/ #define BULK_TEST
#ifdef BULK_TEST
namespace fs = GLib::Compat::filesystem;

void ScanFile(const fs::path & p, std::ostream & s)
{
	std::ifstream t(p);
	if (!t)
	{
		std::cout << "read failed : " + p.u8string() << '\n';
		return;
	}

	try
	{
		std::stringstream ss;
		ss << t.rdbuf();
		Parse(Holder {ss.str()});
	}
	catch (const std::runtime_error & e)
	{
		s << p.u8string() << " : " << e.what() << '\n';
	}
}

BOOST_AUTO_TEST_CASE(BulkTest)
{
	auto paths = {
#if defined(__linux__) && defined(__GNUG__)
		"/usr/include"
#elif defined(_WIN32) && defined(_MSC_VER)
		R"--(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\include)--",
		R"--(C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.24.28314\crt)--",
		R"--(C:\Users\Glen\source\ExternalDependencies\boost_1_69_0_test\boost)--"
#else
#error unknown
#endif
	};

	std::ostringstream s;
	for (auto p : paths)
	{
		size_t count {};
		for (const auto & de : fs::recursive_directory_iterator(p))
		{
			if (is_regular_file(de.path()) && de.path().extension() != ".asm")
			{
				ScanFile(de.path(), s);
				++count;
			}
		}
		std::cout << "BulkTest: " << p << ", Count: " << count << '\n';
	}

	BOOST_TEST("" == s.str());
}
#endif

BOOST_AUTO_TEST_SUITE_END()
