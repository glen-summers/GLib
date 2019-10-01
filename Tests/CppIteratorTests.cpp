#include <boost/test/unit_test.hpp>

#include "../Coverage/CppHtmlGenerator.h"

#include "TestUtils.h"

namespace GLib::Cpp
{
	std::ostream & operator<<(std::ostream & str, const State & s)
	{
		static constexpr std::array<std::string_view, (size_t)State::Count> stateNames
		{
			"Error",
			"None",
			"WhiteSpace",
			"CommentStart",
			"CommentLine",
			"Continuation",
			"CommentBlock",
			"CommentAsterix",
			"Directive",
			"String",
			"RawStringPrefix",
			"RawString",
			"Code",
			"SystemInclude",
			"CharacterLiteral",
			"CharacterEscape"
		};

		return str << stateNames[static_cast<int>(s)];
	}

	std::ostream & operator<<(std::ostream & s, const Fragment & f)
	{
		return s << "State: " << f.first << ", Value: \'" << f.second << '\'';
	}
}

using namespace GLib::Cpp;

void Parse(const Holder & code)
{
	for (auto x : code)
	{
		(void)x;
	}
}

BOOST_AUTO_TEST_SUITE(CppIteratorTests)

BOOST_AUTO_TEST_CASE(Empty)
{
	Holder code = { R"()" };

	std::vector<Fragment> expected {};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Code0)
{
	Holder code = { "void" };

	std::vector<Fragment> expected
	{
		{State::Code, "void" },
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Code1)
{
	Holder code = { R"(void foo)" };

	std::vector<Fragment> expected
	{
		{State::Code, "void" },
		{State::WhiteSpace, " " },
		{State::Code, "foo" }, 
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentBlock)
{
	Holder code = { R"(/**/)" };

	std::vector<Fragment> expected
	{
		{ State::CommentBlock, { "/**/" }}
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentWhiteSpace)
{
	Holder code = { R"(/**/ 
;)" };

	std::vector<Fragment> expected
	{
		{ State::CommentBlock, { "/**/" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "\n;" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentLineContinue)
{
	Holder code = { R"(// hello\
continue
/* block */ /* another block */
)" };

	std::vector<Fragment> expected
	{
		{ State::CommentLine, { "// hello" }},
		{ State::CommentLine, { "\\\ncontinue\n" }},
		{ State::CommentBlock, { "/* block */" }},
		{ State::WhiteSpace, { " " }},
		{ State::CommentBlock, { "/* another block */" }},
		{ State::WhiteSpace, { "\n" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentLineNotContinue)
{
	Holder code = { "// hello \\ not continuation" };

	std::vector<Fragment> expected
	{
		{ State::CommentLine, { "// hello " }},
		{ State::CommentLine, { "\\ not continuation" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentLineNotContinueEnd)
{
	Holder code = { "// hello not continuation \\/" };

	std::vector<Fragment> expected
	{
		{ State::CommentLine, { "// hello not continuation " }},
		{ State::CommentLine, { "\\/" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentStar)
{
	Holder code = { "/* * */" };

	std::vector<Fragment> expected
	{
		{ State::CommentBlock, { "/* * */" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(NotCommentStart)
{
	Holder code = { "int foo=bar/baz;" };

	std::vector<Fragment> expected
	{
		{ State::Code, { "int" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "foo=bar" }},
		{ State::Code, { "/baz;" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CommentFromStateCode)
{
	Holder code = { "bar//comment\n;" };

	std::vector<Fragment> expected
	{
		{ State::Code, { "bar" }},
		{ State::CommentLine, { "//comment\n" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(String)
{
	Holder code = { R"(auto fred = "this is a string";)" };

	std::vector<Fragment> expected
	{
		{ State::Code, { "auto" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "fred"}},
		{ State::WhiteSpace, { " "}},
		{ State::Code, { "=" }},
		{ State::WhiteSpace, { " " }},
		{ State::String, { "\"this is a string\"" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(StringFromStateCode)
{
	Holder code = { R"(;"hello";)" };

	std::vector<Fragment> expected
	{
		{ State::Code, { ";" }},
		{ State::String, { "\"hello\"" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(String2)
{
	Holder code = { R"("\x00 \\")" };

	std::vector<Fragment> expected
	{
		{ State::String, { R"("\x00 \\")" }}
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawString)
{
	Holder code =
	{
		R"--(auto fred = R"(this is a raw string)";)--"
	};

	std::vector<Fragment> expected
	{
		{ State::Code, { "auto" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "fred"}},
		{ State::WhiteSpace, { " "}},
		{ State::Code, { "=" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "R" }},
		{ State::RawString, { "\"(this is a raw string)\"" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawStringPrefix)
{
	Holder code = { R"--(auto fred = R"==(this is a raw string)==";)--" };

	std::vector<Fragment> expected
	{
		{ State::Code, { "auto" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "fred"}},
		{ State::WhiteSpace, { " "}},
		{ State::Code, { "=" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "R" }},
		{ State::RawString, { R"--("==(this is a raw string)==")--" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawStringIgnored)
{
	Holder code = { R"(R"--(hello)--)--")" };

	std::vector<Fragment> expected
	{
		{State::Code, "R" },
		{State::RawString, "\"--(hello)--)--\"" },
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(RawStringPrefixTooLong)
{
	std::string_view  code = R"(R"12345678901234567(content)12345678901234567")";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: '7' (0x37) at line 1 state 0");
}

BOOST_AUTO_TEST_CASE(RawStringPrefixErrorSpace)
{
	std::string_view  code = R"(R" (content) ")";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: ' ' (0x20) at line 1 state 0");
}

BOOST_AUTO_TEST_CASE(RawStringPrefixErrorCloseParenthesis)
{
	std::string_view code = R"--(R")(content)(")--";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: ')' (0x29) at line 1 state 0");
}

BOOST_AUTO_TEST_CASE(RawStringPrefixBackslash)
{
	std::string_view code = R"--(R"\(content)\")--";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code) , "Illegal character: '\\' (0x5c) at line 1 state 0");
}

BOOST_AUTO_TEST_CASE(RawStringNewLine)
{
	Holder code =
	{
		R"--(R"(1
2
3)")--"
	};

	std::vector<Fragment> expected
	{
		{ State::Code, { "R" }},
		{ State::RawString, { R"--("(1
2
3)")--" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(Main)
{
	Holder code
	{
		R"--(#include <iostream>

int main() // main
{
	std::cout << "HelloWorld!" << std::endl;
	std::cout << R"(HelloWorld!)" << std::endl;
	return 0;
}
)--"
	};

	std::vector<Fragment> expected
	{
		{ State::Directive, { "#include " }},
		{ State::SystemInclude, { "<iostream>" }},
		{ State::WhiteSpace, { "\n\n" }},
		{ State::Code, { "int" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "main()" }},
		{ State::WhiteSpace, { " " }},
		{ State::CommentLine, { "// main\n" }},
		{ State::Code, { "{" }},
		{ State::WhiteSpace, { "\n\t" }},

		{ State::Code, { "std::cout" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "<<" }},
		{ State::WhiteSpace, { " " }},
		{ State::String, { R"("HelloWorld!")" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "<<" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "std::endl;" }},
		{ State::WhiteSpace, { "\n\t" }},

		{ State::Code, { "std::cout" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "<<" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "R" }},
		{ State::RawString, { "\"(HelloWorld!)\"" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "<<" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "std::endl;" }},
		{ State::WhiteSpace, { "\n\t" }},

		{ State::Code, { "return" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "0;" }},
		{ State::WhiteSpace, { "\n" }},
		{ State::Code, { "}" }},
		{ State::WhiteSpace, { "\n" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(SystemInclude)
{
	Holder code { R"--(#include <experimental/filesystem>)--" };

	std::vector<Fragment> expected
	{
		{ State::Directive, { "#include " }},
		{ State::SystemInclude, { "<experimental/filesystem>" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CharacterLiteral)
{
	Holder code = { R"(auto char='\"';)" };

	std::vector<Fragment> expected
	{
		{ State::Code, { "auto" }},
		{ State::WhiteSpace, { " " }},
		{ State::Code, { "char=" }},
		{ State::CharacterLiteral, { R"('\"')" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CharacterLiteralFromStateNone)
{
	Holder code = { R"('\x00';)" };

	std::vector<Fragment> expected
	{
		{ State::CharacterLiteral, { R"('\x00')" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CharacterLiteralFromStateWhitespace)
{
	Holder code = { R"( '\x00';)" };

	std::vector<Fragment> expected
	{
		{ State::WhiteSpace, { " " }},
		{ State::CharacterLiteral, { R"('\x00')" }},
		{ State::Code, { ";" }},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(CharacterEscapeError)
{
	std::string_view code = "'\\\n;";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: '\n' (0xa) at line 1 state 0");
}

BOOST_AUTO_TEST_CASE(Guard)
{
	Holder code = { R"(/* comment */
#ifndef file_included // another comment
#define file_included

#endif /* not file_included */
)" };

	std::vector<Fragment> expected
	{
		{ State::CommentBlock, "/* comment */" },
		{ State::WhiteSpace, "\n" },
		{ State::Directive, "#ifndef file_included " },
		{ State::CommentLine, "// another comment\n" },
		{ State::Directive, "#define file_included" },
		{ State::WhiteSpace, "\n\n" },
		{ State::Directive, "#endif " },
		{ State::CommentBlock, "/* not file_included */" },
		{ State::WhiteSpace, "\n" },
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(ContinueError)
{
	std::string_view code = R"(# include \x)";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Illegal character: 'x' (0x78) at line 1 state 0");
}

BOOST_AUTO_TEST_CASE(DirectiveContinue)
{
	Holder code = { R"(#include \
"foo";)" };

	std::vector<Fragment> expected
	{
		{ State::Directive, "#include " },
		{ State::Directive, "\\\n" },
		{ State::String, R"("foo")" },
		{ State::Code, ";" },
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), code.begin(), code.end());
}

BOOST_AUTO_TEST_CASE(TerminationError)
{
	std::string_view code = R"("stringNotClosed)";

	GLIB_CHECK_RUNTIME_EXCEPTION(Parse(code), "Termination error, State: 9");
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

	auto expected = "<span class=\"d\">#include\xC2\xB7</span><span class=\"s\">&quot;foo.h&quot;</span>";

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

	auto expected = "<span class=\"k\">auto</span>"\
		"<span class=\"w\">\xC2\xB7</span>"\
		"v="\
		"std::<span class=\"t\">vector</span>"\
		"{};";

	TestUtils::Compare(stm.str(), expected);
}

BOOST_AUTO_TEST_SUITE_END()
