#include <boost/test/unit_test.hpp>

#include "GLib/Xml/Printer.h"

#include "TestUtils.h"
#include "XmlTestUtils.h"

using namespace GLib;

BOOST_AUTO_TEST_SUITE(XmlStateEngineTests)

BOOST_AUTO_TEST_CASE(EmptyElement)
{
	Xml::Holder xml { "<xml/>" };

	std::vector<Xml::Element> expected
	{
		{"xml", Xml::ElementType::Empty},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(ElementSpace)
{
	Xml::Holder xml { "<xml									/>" };

	std::vector<Xml::Element> expected
	{
		{"xml", Xml::ElementType::Empty, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(EndElementSpace)
{
	Xml::Holder xml { "<xml></xml							>" };

	std::vector<Xml::Element> expected
	{
		{"xml", Xml::ElementType::Open, {}},
		{"xml", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(Element)
{
	Xml::Holder xml { "<xml></xml>" };

	std::vector<Xml::Element> expected
	{
		{"xml", Xml::ElementType::Open, {}},
		{"xml", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(SubElementExtra)
{
	Xml::Holder xml { "<xml><sub/></xml>" };

	std::vector<Xml::Element> expected
	{
		{"xml", Xml::ElementType::Open, {}},
		{"sub", Xml::ElementType::Empty, {}},
		{"xml", Xml::ElementType::Close, {}},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}


BOOST_AUTO_TEST_CASE(SubElement)
{
	Xml::Holder xml { "<xml><sub/></xml>" };

	std::vector<Xml::Element> expected
	{
		{"xml", Xml::ElementType::Open, {}},
		{"sub", Xml::ElementType::Empty, {}},
		{"xml", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

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

BOOST_AUTO_TEST_CASE(IterateAttributes)
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

	// std::vector<Xml::Element> actual {xml.begin(), xml.end()}; error
	std::vector<Xml::Element> actual;
	for (const auto & e : xml)
	{
		actual.push_back(e);
	}

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());

	std::vector<Xml::Attribute> expectedAttr0 = { {"a", "1", "", "a='1'"}, {"b", "2", "", "b='2'"} };
	BOOST_CHECK_EQUAL_COLLECTIONS(expectedAttr0.begin(), expectedAttr0.end(),
		actual[0].Attributes().begin(), actual[0].Attributes().end());

	std::vector<Xml::Attribute> expectedAttr1 = { {"c", "3", "", "c='3'"}, {"d", "4", "", "d=\"4\""} };
	BOOST_CHECK_EQUAL_COLLECTIONS(expectedAttr1.begin(), expectedAttr1.end(),
		actual[1].Attributes().begin(), actual[1].Attributes().end());
}

BOOST_AUTO_TEST_CASE(AttributeSpace)
{
	Xml::Holder xml { "<root a = '1' b  =  '2'/>" };

	std::vector<Xml::Element> expected
	{
		{"root", Xml::ElementType::Empty, { "a = '1' b  =  '2'" }},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(OuterXml)
{
	Xml::Holder xml { R"(<root a='1' b='2' >
	<sub c='3' d=""/>
</root>)" };

	auto it = xml.begin();
	BOOST_CHECK(it->Name()=="root" && it->OuterXml() == "<root a='1' b='2' >");
	++it;
	BOOST_CHECK(it->Name()=="sub" && it->OuterXml() == R"(
	<sub c='3' d=""/>)");
	++it;
	BOOST_CHECK(it->Name()=="root" && it->OuterXml() == R"(
</root>)");
	BOOST_CHECK(++it == xml.end());
}

BOOST_AUTO_TEST_CASE(XmlDecl)
{
	Xml::Holder xml = { R"(<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE greeting SYSTEM "hello.dtd">
<greeting>Hello, world!</greeting>)" };

	std::vector<Xml::Element> expected
	{
		Xml::Element{"greeting", Xml::ElementType::Open, {}},
		Xml::Element{Xml::ElementType::Text, "Hello, world!"},
		Xml::Element{"greeting", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(XmlDeclMustBeFirst)
{
	auto xml = R"(<!-- baad -->
<?xml version="1.0"?>
<xml/>)";

	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse(xml); }, "Illegal character: '?' (0x3f)");
}

BOOST_AUTO_TEST_CASE(DefaultNameSpaceToDo)
{
	Xml::Holder xml{ R"(<xml xmlns='foo'/>)"};

	std::vector<Xml::Element> expected
	{
		// Xml::Element{"xml", "xml", "foo", Xml::ElementType::Empty, {}},
		Xml::Element{"xml", "xml", "", Xml::ElementType::Empty, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(NameSpace)
{
	Xml::Holder xml{ R"(<foo:x foo:bar='baz' xmlns:foo='foo-ns'/>)"};

	std::vector<Xml::Element> expected
	{
		{"foo:x", "x", "foo-ns", Xml::ElementType::Empty, {"foo:bar='baz'"}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(NameSpace2)
{
	Xml::Holder xml{ R"(<foo:x xmlns:foo='foo-ns'>
	<bar:y xmlns:bar='bar-ns' foo:at='f' bar:at='b'/>
</foo:x>
)"};

	std::vector<Xml::Element> expected
	{
		{"foo:x", "x", "foo-ns", Xml::ElementType::Open, {}},
		{"bar:y", "y", "bar-ns", Xml::ElementType::Empty, { "xmlns:bar='bar-ns' foo:at='f' bar:at='b'" }},
		{"foo:x", "x", "foo-ns", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(NamespaceRedefine)
{
	Xml::Holder xml{ R"(<foo:x xmlns:foo='foo-ns' xmlns:bar='bar-ns'>
	<foo:y xmlns:foo='newfoo-ns'/>
	<foo:z xmlns:foo='newnewfoo-ns'/>
	<bar:b0>
		<bar:b1 xmlns:bar='nubar-ns'>
		</bar:b1>
	</bar:b0>
</foo:x>
)"};

	std::vector<Xml::Element> expected
	{
		{"foo:x", "x", "foo-ns", Xml::ElementType::Open, {}},
		{"foo:y", "y", "newfoo-ns", Xml::ElementType::Empty, {}},
		{"foo:z", "z", "newnewfoo-ns", Xml::ElementType::Empty, {}},
		{"bar:b0", "b0", "bar-ns", Xml::ElementType::Open, {}},
		{"bar:b1", "b1", "nubar-ns", Xml::ElementType::Open, {}},
		{"bar:b1", "b1", "nubar-ns", Xml::ElementType::Close, {}},
		{"bar:b0", "b0", "bar-ns", Xml::ElementType::Close, {}},
		{"foo:x", "x", "foo-ns", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(NamespaceRedefine2)
{
	Xml::Holder xml{ R"(<foo:x xmlns:foo='foo-ns' xmlns:bar='bar-ns'>
	<b0 xmlns:foo='newfoo-ns' xmlns:bar='newbar-ns'>
		<foo:f/>
		<bar:b/>
	</b0>
</foo:x>
)"};

	std::vector<Xml::Element> expected
	{
		{"foo:x", "x", "foo-ns", Xml::ElementType::Open, {}},
		{"b0", "b0", "", Xml::ElementType::Open, {}},
		{"foo:f", "f", "newfoo-ns", Xml::ElementType::Empty, {}},
		{"bar:b", "b", "newbar-ns", Xml::ElementType::Empty, {}},
		{"b0", "b0", "", Xml::ElementType::Close, {}},
		{"foo:x", "x", "foo-ns", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(CommentAtStart)
{
	Xml::Holder xml { R"(<!-- Comment -->
<Xml/>)"};

	std::vector<Xml::Element> expected
	{
		{Xml::ElementType::Comment, " Comment "},
		{"Xml", Xml::ElementType::Empty, {}}
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(TwoCommentsAtStartWouldBeAnExtravegance)
{
	Xml::Holder xml { R"(<!-- Comment -->
<!-- Comment -->
<Xml/>)"};

	std::vector<Xml::Element> expected
	{
		{Xml::ElementType::Comment, " Comment "},
		{Xml::ElementType::Comment, " Comment "},
		{"Xml", Xml::ElementType::Empty, {}}
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(CommentAtEnd)
{
	Xml::Holder xml { R"(<Xml/>
<!-- Comment -->)"};

	std::vector<Xml::Element> expected
	{
		{"Xml", Xml::ElementType::Empty, {} },
		{Xml::ElementType::Comment, " Comment "}
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(AbundanceOfComments)
{
	Xml::Holder xml { R"(<!-- Comment1 -->
<Xml>
<!-- Comment2 -->
</Xml>
<!-- Comment3 -->
)"};

	std::vector<Xml::Element> expected
	{
		Xml::Element{Xml::ElementType::Comment, " Comment1 "},
		Xml::Element{"Xml", Xml::ElementType::Open, {}},
		Xml::Element{Xml::ElementType::Comment, " Comment2 "},
		Xml::Element{"Xml", Xml::ElementType::Close, {}},
		Xml::Element{Xml::ElementType::Comment, " Comment3 "},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(DocType)
{
	Xml::Holder xml{ R"(<!DOCTYPE blah>
<Xml/>)"};

	std::vector<Xml::Element> expected
	{
		{"Xml", "Xml", "", Xml::ElementType::Empty, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(DocTypeTwiceIsError)
{
	auto xml { R"(<!DOCTYPE blah>
<!DOCTYPE blah>
<Xml/>)"};

	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse(xml); }, "Illegal character: 'D' (0x44)");
}

BOOST_AUTO_TEST_CASE(CData)
{
	Xml::Holder xml = { R"(<xml>
	<![CDATA[<greeting>Hello, world!</greeting>]]>
</xml>
)" };

	std::vector<Xml::Element> expected
	{
		Xml::Element{"xml", Xml::ElementType::Open, {}},
		Xml::Element{"xml", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(CDataOkWithRightSquareBrackets)
{
	Xml::Holder xml = { R"(<xml>
	<![CDATA[<greeting>Hello, world! ] ]] </greeting>]]>
</xml>
)" };

	std::vector<Xml::Element> expected
	{
		Xml::Element{"xml", Xml::ElementType::Open, {}},
		Xml::Element{"xml", Xml::ElementType::Close, {}},
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

BOOST_AUTO_TEST_CASE(ExtraContentAtEndThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<xml/><extra>"); }, "Extra content at document end");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<xml/>    <extra>"); }, "Extra content at document end");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<xml/>  <!-- comment -->  <extra>"); }, "Extra content at document end");

	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<xml/><extra/>"); }, "Extra content at document end");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<xml/></extra>"); }, "Extra content at document end");
}

BOOST_AUTO_TEST_CASE(TextNodeAtRootThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("baad"); },"Illegal character: 'b' (0x62)");
}

BOOST_AUTO_TEST_CASE(MismatchedElementsThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<root></notRoot>"); }, "Element mismatch: notRoot != root");
}

BOOST_AUTO_TEST_CASE(MissingAttrSpaceThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<root at1='val1'at2='val2'/>"); }, "Illegal character: 'a' (0x61)");
}

BOOST_AUTO_TEST_CASE(NotClosed)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x>"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x >"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x a"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x a="); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x a='"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x a='1"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x a='1'"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("<x a='1'/"); }, "Xml not closed");
}

BOOST_AUTO_TEST_CASE(EmptyIsError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse(""); };, "No root element");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Xml::Parse("    			    "); };, "No root element");
}

BOOST_AUTO_TEST_CASE(BaadColons)
{
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<:x/>"); }, "Illegal name : ':x'");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x::y/>"); }, "Illegal name : 'x::y'");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x:-:y/>"); }, "Illegal name : 'x:-:y'");
}

BOOST_AUTO_TEST_CASE(MalformedComments)
{
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<!- Comment1 --><Xml/>"); }, "Illegal character: ' ' (0x20)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<Xml><!- Comment1 --></Xml>"); }, "Illegal character: ' ' (0x20)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<Xml><!-- Comment1 -- --></Xml>"); }, "Illegal character: ' ' (0x20)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<Xml><!-- Comment1 -></Xml>"); }, "Xml not closed");
}

BOOST_AUTO_TEST_CASE(ElementEndError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x!"); }, "Illegal character: '!' (0x21)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x !"); }, "Illegal character: '!' (0x21)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x/!"); }, "Illegal character: '!' (0x21)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x></!"); }, "Illegal character: '!' (0x21)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x></x!"); }, "Illegal character: '!' (0x21)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x></x !"); }, "Illegal character: '!' (0x21)");
}

BOOST_AUTO_TEST_CASE(AttributeNameError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x a!='1' />"); }, "Illegal character: '!' (0x21)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x a !='1' />"); }, "Illegal character: '!' (0x21)");
}

BOOST_AUTO_TEST_CASE(AttributeValueError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x a = !'1' />"); }, "Illegal character: '!' (0x21)");

	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x a='<' />"); }, "Illegal character: '<' (0x3c)");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse(R"(<x a="<" />)"); }, "Illegal character: '<' (0x3c)");

	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<x a='&' />"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse(R"(<x a="&" />)"); }, "Xml not closed");
}

BOOST_AUTO_TEST_CASE(ErrorMeansError)
{
	Xml::StateEngine engine;
	BOOST_TEST(engine.GetState() == Xml::State::Start);
	engine.Push('!');
	BOOST_TEST(engine.GetState() == Xml::State::Error);
	engine.Push('!');
	BOOST_TEST(engine.GetState() == Xml::State::Error);
}

BOOST_AUTO_TEST_CASE(EndOfTheWorld)
{
	Xml::Holder xml( "<xml/>");
	auto end = xml.end();
	GLIB_CHECK_RUNTIME_EXCEPTION( { ++end; }, "++end");
}

BOOST_AUTO_TEST_CASE(ElementNamespaceNotFound)
{
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<foo:xml bar='fubard'/>"); }, "NameSpace foo not found");
}

BOOST_AUTO_TEST_CASE(AttrNamespaceNotFound)
{
	GLIB_CHECK_RUNTIME_EXCEPTION( { Xml::Parse("<xml bar:baz='barbazd'/>"); }, "NameSpace bar not found");
}

BOOST_AUTO_TEST_CASE(Entities)
{
/* TODO have iterator convert standard entites rather than just passing them on
	CharRef: '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'
	EntityRef: '&' Name '; [amp, lt, gt, apos, quot]
*/

	Xml::Parse("<xml>&#x20ac;</xml>");
	Xml::Parse("<xml>&#8364;</xml>");
	Xml::Parse("<xml>&amp; &lt; &gt; &apos; &quot;</xml>");
}

BOOST_AUTO_TEST_CASE(AttributeEntity)
{
	Xml::Attributes attr("attr='&customEntity;'");

	std::vector<Xml::Attribute> expected
	{
		{"attr", "&customEntity;", "", "attr='&customEntity;'"}
	};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), attr.begin(), attr.end());
}

// move to another file
BOOST_AUTO_TEST_CASE(AttributeIteratorAll)
{
	Xml::Attributes attr { "a='1' b='2' xmlns:foo='bar'" };

	std::vector<Xml::Attribute> expected
	{
		{"a", "1", {}, "a='1'"},
		{"b", "2", {}, "b='2'"},
		{"xmlns:foo", "bar", {}, "xmlns:foo='bar'"},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), attr.begin(), attr.end());
}

BOOST_AUTO_TEST_CASE(AttributeIteratorEnum)
{
	Xml::NameSpaceManager man;
	Xml::Attributes attr { "a='1' b='2' xmlns:foo='bar'", &man };

	std::vector<Xml::Attribute> expected
	{
		{"a", "1", {}, "a='1'"},
		{"b", "2", {}, "b='2'"},
	};

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), attr.begin(), attr.end());
}

	BOOST_AUTO_TEST_CASE(PrinterEscapes) // move, expand
	{
		GLib::Xml::Printer p;
		p.PushText("Start && End");
		BOOST_TEST("Start &amp;&amp; End" == p.Xml());
	}

	BOOST_AUTO_TEST_CASE(PrinterFormat)
	{
		{
			GLib::Xml::Printer formatted{ true };
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
			GLib::Xml::Printer unFormatted{ false };
			unFormatted.OpenElement("Root");
			unFormatted.OpenElement("Nested");
			unFormatted.CloseElement();
			unFormatted.CloseElement();
			auto xmlUnFormatted = unFormatted.Xml();
			BOOST_TEST("<Root><Nested/></Root>" == xmlUnFormatted);
		}

		{
			GLib::Xml::Printer unFormatted2{ true };
			unFormatted2.OpenElement("Root", false);
			unFormatted2.OpenElement("Nested", false);
			unFormatted2.CloseElement(false);
			unFormatted2.CloseElement(false);
			auto xmlUnFormatted = unFormatted2.Xml();
			BOOST_TEST("<Root><Nested/></Root>" == xmlUnFormatted);
		}
	}

BOOST_AUTO_TEST_SUITE_END()