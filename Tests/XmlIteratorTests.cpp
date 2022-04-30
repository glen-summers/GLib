
#include <GLib/Xml/Printer.h>

#include <boost/test/unit_test.hpp>

#include "TestUtils.h"
#include "XmlTestUtils.h"

using GLib::Xml::Attribute;
using GLib::Xml::Attributes;
using GLib::Xml::Element;
using GLib::Xml::ElementType;
using GLib::Xml::Holder;
using GLib::Xml::Parse;
using GLib::Xml::Printer;

AUTO_TEST_SUITE(XmlStateEngineTests)

AUTO_TEST_CASE(EmptyElement)
{
	Holder xml {"<xml/>"};

	std::vector<Element> expected {{"xml", ElementType::Empty}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
	CHECK(xml.begin()->GetAttributes().Empty());
}

AUTO_TEST_CASE(ElementSpace)
{
	Holder xml {"<xml									/>"};

	std::vector<Element> expected {{"xml", ElementType::Empty, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(EndElementSpace)
{
	Holder xml {"<xml></xml							>"};

	std::vector<Element> expected {{"xml", ElementType::Open, {}}, {"xml", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(ElementTest)
{
	Holder xml {"<xml></xml>"};

	std::vector<Element> expected {{"xml", ElementType::Open, {}}, {"xml", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(SubElementExtra)
{
	Holder xml {"<xml><sub/></xml>"};

	std::vector<Element> expected {{"xml", ElementType::Open, {}}, {"sub", ElementType::Empty, {}}, {"xml", ElementType::Close, {}}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(SubElement)
{
	Holder xml {"<xml><sub/></xml>"};

	std::vector<Element> expected {{"xml", ElementType::Open, {}}, {"sub", ElementType::Empty, {}}, {"xml", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(AttributesTest)
{
	Holder xml {R"(<root a='1' b='2'>
	<sub c='3' d="4"/>
</root>)"};

	std::vector<Element> expected {{"root", ElementType::Open, Attributes {"a='1' b='2'"}},
																 {"sub", ElementType::Empty, Attributes {"c='3' d=\"4\""}},
																 {"root", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(IterateAttributes)
{
	Holder xml {R"(<root a='1' b='2'>
	<sub c='3' d="4"/>
</root>)"};

	std::vector<Element> expected {{"root", ElementType::Open, Attributes {"a='1' b='2'"}},
																 {"sub", ElementType::Empty, Attributes {"c='3' d=\"4\""}},
																 {"root", ElementType::Close, Attributes {}}};

	// std::vector<Element> actual {xml.begin(), xml.end()}; error
	std::vector<Element> actual;
	for (const auto & e : xml)
	{
		actual.push_back(e);
	}

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());

	std::vector<Attribute> expectedAttr0 = {{"a", "1", "", "a='1'"}, {"b", "2", "", "b='2'"}};
	CHECK_EQUAL_COLLECTIONS(expectedAttr0.begin(), expectedAttr0.end(), actual[0].GetAttributes().begin(), actual[0].GetAttributes().end());

	std::vector<Attribute> expectedAttr1 = {{"c", "3", "", "c='3'"}, {"d", "4", "", "d=\"4\""}};
	CHECK_EQUAL_COLLECTIONS(expectedAttr1.begin(), expectedAttr1.end(), actual[1].GetAttributes().begin(), actual[1].GetAttributes().end());
}

AUTO_TEST_CASE(AttributeSpace)
{
	Holder xml {"<root a = '1' b = '2' />"};

	std::vector<Element> expected {{"root", ElementType::Empty, Attributes {"a = '1' b = '2'"}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(OuterXml)
{
	Holder xml {R"(<root a='1' b='2' >
	<sub c='3' d=""/>
</root>)"};

	auto it = xml.begin();
	CHECK(it->Name() == "root" && it->OuterXml() == "<root a='1' b='2' >");
	++it;
	CHECK(it->Name() == "sub" && it->OuterXml() == R"(
	<sub c='3' d=""/>)");
	++it;
	CHECK(it->Name() == "root" && it->OuterXml() == R"(
</root>)");
	CHECK(++it == xml.end());
}

AUTO_TEST_CASE(XmlDecl)
{
	Holder xml {R"(<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE greeting SYSTEM "hello.dtd">
<greeting>Hello, world!</greeting>)"};

	std::vector<Element> expected {{"greeting", ElementType::Open, {}}, {ElementType::Text, "Hello, world!"}, {"greeting", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(XmlDeclMustBeFirst)
{
	const auto * xml = R"(<!-- baad -->
<?xml version="1.0"?>
<xml/>)";

	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse(xml); }, "Illegal character: '?' (0x3f) at line: 1, offset: 1");
}

AUTO_TEST_CASE(DefaultNameSpaceToDo)
{
	Holder xml {R"(<xml xmlns='foo'/>)"};

	std::vector<Element> expected {{"xml", "xml", "", ElementType::Empty, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(NameSpace)
{
	Holder xml {R"(<foo:x foo:bar='baz' xmlns:foo='foo-ns'/>)"};

	std::vector<Element> expected {{"foo:x", "x", "foo-ns", ElementType::Empty, Attributes {"foo:bar='baz'"}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(NameSpace2)
{
	Holder xml {R"(<foo:x xmlns:foo='foo-ns'>
	<bar:y xmlns:bar='bar-ns' foo:at='f' bar:at='b'/>
</foo:x>
)"};

	std::vector<Element> expected {{"foo:x", "x", "foo-ns", ElementType::Open, {}},
																 {"bar:y", "y", "bar-ns", ElementType::Empty, Attributes {"xmlns:bar='bar-ns' foo:at='f' bar:at='b'"}},
																 {"foo:x", "x", "foo-ns", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(NamespaceRedefine)
{
	Holder xml {R"(<foo:x xmlns:foo='foo-ns' xmlns:bar='bar-ns'>
	<foo:y xmlns:foo='newfoo-ns'/>
	<foo:z xmlns:foo='newnewfoo-ns'/>
	<bar:b0>
		<bar:b1 xmlns:bar='nubar-ns'>
		</bar:b1>
	</bar:b0>
</foo:x>
)"};

	std::vector<Element> expected {
		{"foo:x", "x", "foo-ns", ElementType::Open, {}},				{"foo:y", "y", "newfoo-ns", ElementType::Empty, {}},
		{"foo:z", "z", "newnewfoo-ns", ElementType::Empty, {}}, {"bar:b0", "b0", "bar-ns", ElementType::Open, {}},
		{"bar:b1", "b1", "nubar-ns", ElementType::Open, {}},		{"bar:b1", "b1", "nubar-ns", ElementType::Close, {}},
		{"bar:b0", "b0", "bar-ns", ElementType::Close, {}},			{"foo:x", "x", "foo-ns", ElementType::Close, {}},
	};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(NamespaceRedefine2)
{
	Holder xml {R"(<foo:x xmlns:foo='foo-ns' xmlns:bar='bar-ns'>
	<b0 xmlns:foo='newfoo-ns' xmlns:bar='newbar-ns'>
		<foo:f/>
		<bar:b/>
	</b0>
</foo:x>
)"};

	std::vector<Element> expected {
		{"foo:x", "x", "foo-ns", ElementType::Open, {}},
		{"b0", "b0", "", ElementType::Open, {}},
		{"foo:f", "f", "newfoo-ns", ElementType::Empty, {}},
		{"bar:b", "b", "newbar-ns", ElementType::Empty, {}},
		{"b0", "b0", "", ElementType::Close, {}},
		{"foo:x", "x", "foo-ns", ElementType::Close, {}},
	};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(CommentAtStart)
{
	Holder xml {R"(<!-- Comment -->
<Xml/>)"};

	std::vector<Element> expected {{ElementType::Comment, "<!-- Comment -->"}, {"Xml", ElementType::Empty, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(TwoCommentsAtStartWouldBeAnExtravegance)
{
	Holder xml {R"(<!-- Comment -->
<!-- Comment -->
<Xml/>)"};

	std::vector<Element> expected {
		{ElementType::Comment, "<!-- Comment -->"}, {ElementType::Comment, "\n<!-- Comment -->"}, {"Xml", ElementType::Empty, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(CommentAtEnd)
{
	Holder xml {R"(<Xml/>
<!-- Comment -->)"};

	std::vector<Element> expected {{"Xml", ElementType::Empty, {}}, {ElementType::Comment, "\n<!-- Comment -->"}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(AbundanceOfComments)
{
	Holder xml {R"(<!-- Comment1 -->
<Xml>
<!-- Comment2 -->
</Xml>
<!-- Comment3 -->
)"};

	std::vector<Element> expected {
		{ElementType::Comment, "<!-- Comment1 -->"},	 {"Xml", ElementType::Open, {}},
		{ElementType::Comment, "\n<!-- Comment2 -->"}, {"Xml", ElementType::Close, {}},
		{ElementType::Comment, "\n<!-- Comment3 -->"},
	};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(DocType)
{
	Holder xml {R"(<!DOCTYPE blah>
<Xml/>)"};

	std::vector<Element> expected {{"Xml", "Xml", "", ElementType::Empty, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(DocTypeTwiceIsError)
{
	const auto * xml {R"(<!DOCTYPE blah>
<!DOCTYPE blah>
<Xml/>)"};

	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse(xml); }, "Illegal character: 'D' (0x44) at line: 1, offset: 2");
}

AUTO_TEST_CASE(CData)
{
	Holder xml {R"(<xml>
	<![CDATA[<greeting>Hello, world!</greeting>]]>
</xml>
)"};

	std::vector<Element> expected {{"xml", ElementType::Open, {}}, {"xml", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(CDataOkWithRightSquareBrackets)
{
	Holder xml {R"(<xml>
	<![CDATA[<greeting>Hello, world! ] ]] </greeting>]]>
</xml>
)"};

	std::vector<Element> expected {{"xml", ElementType::Open, {}}, {"xml", ElementType::Close, {}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(ExtraContentAtEndThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<xml/><extra>"); }, "Extra content at document end");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<xml/>    <extra>"); }, "Extra content at document end");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<xml/>  <!-- comment -->  <extra>"); }, "Extra content at document end");

	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<xml/><extra/>"); }, "Extra content at document end");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<xml/></extra>"); }, "Extra content at document end");
}

AUTO_TEST_CASE(TextNodeAtRootThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("baad"); }, "Illegal character: 'b' (0x62) at line: 0, offset: 0");
}

AUTO_TEST_CASE(MismatchedElementsThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<root></notRoot>"); }, "Element mismatch: notRoot != root, at line: 0, offset: 16");
}

AUTO_TEST_CASE(MissingAttrSpaceThrows)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<root at1='val1'at2='val2'/>"); }, "Illegal character: 'a' (0x61) at line: 0, offset: 16");
}

AUTO_TEST_CASE(NotClosed)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x>"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x >"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a="); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a='"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a='1"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a='1'"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a='1'/"); }, "Xml not closed");
}

AUTO_TEST_CASE(EmptyIsError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse(""); };, "No root element");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("    			    "); };, "No root element");
}

AUTO_TEST_CASE(BaadColons)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<:x/>"); }, "Illegal name : ':x'");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x::y/>"); }, "Illegal name : 'x::y'");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x:-:y/>"); }, "Illegal name : 'x:-:y'");
}

AUTO_TEST_CASE(MalformedComments)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<!- Comment1 --><Xml/>"); }, "Illegal character: ' ' (0x20) at line: 0, offset: 3");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<Xml><!- Comment1 --></Xml>"); }, "Illegal character: ' ' (0x20) at line: 0, offset: 8");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<Xml><!-- Comment1 -- --></Xml>"); }, "Illegal character: ' ' (0x20) at line: 0, offset: 21");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<Xml><!-- Comment1 -></Xml>"); }, "Xml not closed");
}

AUTO_TEST_CASE(ElementEndError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x!"); }, "Illegal character: '!' (0x21) at line: 0, offset: 2");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x !"); }, "Illegal character: '!' (0x21) at line: 0, offset: 3");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x/!"); }, "Illegal character: '!' (0x21) at line: 0, offset: 3");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x></!"); }, "Illegal character: '!' (0x21) at line: 0, offset: 5");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x></x!"); }, "Illegal character: '!' (0x21) at line: 0, offset: 6");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x></x !"); }, "Illegal character: '!' (0x21) at line: 0, offset: 7");
}

AUTO_TEST_CASE(AttributeNameError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a!='1' />"); }, "Illegal character: '!' (0x21) at line: 0, offset: 4");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a !='1' />"); }, "Illegal character: '!' (0x21) at line: 0, offset: 5");
}

AUTO_TEST_CASE(AttributeValueError)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a = !'1' />"); }, "Illegal character: '!' (0x21) at line: 0, offset: 7");

	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a='<' />"); }, "Illegal character: '<' (0x3c) at line: 0, offset: 6");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse(R"(<x a="<" />)"); }, "Illegal character: '<' (0x3c) at line: 0, offset: 6");

	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<x a='&' />"); }, "Xml not closed");
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse(R"(<x a="&" />)"); }, "Xml not closed");
}

AUTO_TEST_CASE(ErrorMeansError)
{
	GLib::Xml::StateEngine engine;
	TEST(engine.GetState() == GLib::Xml::State::Start);
	engine.Push('!');
	TEST(engine.GetState() == GLib::Xml::State::Error);
	engine.Push('!');
	TEST(engine.GetState() == GLib::Xml::State::Error);
}

AUTO_TEST_CASE(EndOfTheWorld)
{
	Holder xml("<xml/>");
	auto end = xml.end();
	GLIB_CHECK_RUNTIME_EXCEPTION({ ++end; }, "++end");
}

AUTO_TEST_CASE(ElementNamespaceNotFound)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<foo:xml bar='fubard'/>"); }, "NameSpace foo not found");
}

AUTO_TEST_CASE(AttrNamespaceNotFound)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ Parse("<xml bar:baz='barbazd'/>"); }, "NameSpace bar not found");
}

AUTO_TEST_CASE(ElementTextEntities)
{
	/* TODO have iterator convert standard entities rather than just passing them on
		CharRef: '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'
		EntityRef: '&' Name '; [amp, lt, gt, apos, quot]
	*/

	Holder xml {"<xml>&amp; &lt; &gt; &apos; &quot; &#x20ac; &#8364;</xml>"};

	std::vector<Element> expected {
		{"xml", ElementType::Open, {}},
		{ElementType::Text, "&amp; &lt; &gt; &apos; &quot; &#x20ac; &#8364;"},
		{"xml", ElementType::Close, {}},
	};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(AttributeEntities)
{
	Holder xml {"<xml attr='&lt;'/>"};

	std::vector<Element> expected {{"xml", ElementType::Empty, Attributes {"attr='&lt;'"}}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), xml.begin(), xml.end());
}

AUTO_TEST_CASE(AttributeEntity)
{
	Attributes attr("attr='&customEntity;'");

	std::vector<Attribute> expected {{"attr", "&customEntity;", "", "attr='&customEntity;'"}};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), attr.begin(), attr.end());
}

// move to another file
AUTO_TEST_CASE(AttributeIteratorAll)
{
	Attributes attr {"a='1' b='2' xmlns:foo='bar'"};

	std::vector<Attribute> expected {{"a", "1", {}, "a='1'"}, {"b", "2", {}, "b='2'"}, {"xmlns:foo", "bar", {}, "xmlns:foo='bar'"}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), attr.begin(), attr.end());
}

AUTO_TEST_CASE(AttributeIteratorEnum)
{
	GLib::Xml::NameSpaceManager man;
	Attributes attr {"a='1' b='2' xmlns:foo='bar'", &man};

	std::vector<Attribute> expected {{"a", "1", {}, "a='1'"}, {"b", "2", {}, "b='2'"}};

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), attr.begin(), attr.end());
}

AUTO_TEST_CASE(AttributeIteratorInvalidChars)
{
	GLIB_CHECK_RUNTIME_EXCEPTION(
		for (const auto & a
				 : Attributes {"0='baad'"}) { static_cast<void>(a); };
		, "Illegal character: '0' (0x30)");

	GLIB_CHECK_RUNTIME_EXCEPTION(
		for (const auto & a
				 : Attributes {"baad=baad"}) { static_cast<void>(a); };
		, "Illegal character: 'b' (0x62)");

	GLIB_CHECK_RUNTIME_EXCEPTION(
		for (const auto & a
				 : Attributes {"baad='-<-'"}) { static_cast<void>(a); };
		, "Illegal character: '<' (0x3c)");
}

AUTO_TEST_CASE(AttributeIteratorEnd)
{
	GLIB_CHECK_RUNTIME_EXCEPTION(++Attributes {}.begin();, "++end");
}

// test comment, text, attributes with entities and combos

AUTO_TEST_CASE(PrinterEscapes) // move, expand
{
	Printer p;
	p.PushText("Start && End");
	TEST("Start &amp;&amp; End" == p.Xml());
}

AUTO_TEST_CASE(PrinterFormat)
{
	{
		Printer formatted {true};
		formatted.OpenElement("Root");
		formatted.OpenElement("Nested");
		formatted.CloseElement();
		formatted.CloseElement();
		std::string xmlFormatted = formatted.Xml();
		TEST(R"(<Root>
 <Nested/>
</Root>
)" == xmlFormatted);
	}

	{
		Printer unFormatted {false};
		unFormatted.OpenElement("Root");
		unFormatted.OpenElement("Nested");
		unFormatted.CloseElement();
		unFormatted.CloseElement();
		auto xmlUnFormatted = unFormatted.Xml();
		TEST("<Root><Nested/></Root>" == xmlUnFormatted);
	}

	{
		Printer unFormatted2 {true};
		unFormatted2.OpenElement("Root", false);
		unFormatted2.OpenElement("Nested", false);
		unFormatted2.CloseElement(false);
		unFormatted2.CloseElement(false);
		auto xmlUnFormatted = unFormatted2.Xml();
		TEST("<Root><Nested/></Root>" == xmlUnFormatted);
	}
}

AUTO_TEST_SUITE_END()