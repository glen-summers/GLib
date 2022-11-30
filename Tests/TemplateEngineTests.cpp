
#include <GLib/Html/TemplateEngine.h>

#include <boost/test/unit_test.hpp>

#include "TestStructs.h"
#include "TestUtils.h"

using GLib::Eval::Evaluator;
using GLib::Html::Generate;

AUTO_TEST_SUITE(TemplateEngineTests)

AUTO_TEST_CASE(SimpleProperty)
{
	Evaluator evaluator;
	evaluator.Set<std::string>("name", "fred");

	std::ostringstream stm;
	Generate(evaluator, "<xml attr='${name}' />", stm);

	TEST(stm.str() == "<xml attr='fred' />");
}

AUTO_TEST_CASE(Nop)
{
	Evaluator evaluator;
	std::ostringstream stm;
	Generate(evaluator, "<xml xmlns:gl='glib'/>", stm);
	TEST(stm.str() == "<xml/>");
}

AUTO_TEST_CASE(Nop2)
{
	Evaluator evaluator;
	std::ostringstream stm;
	Generate(evaluator, "<xml xmlns:gl1='glib' xmlns:gl2='glib'/>", stm);
	TEST(stm.str() == "<xml/>");
}

AUTO_TEST_CASE(ForEach)
{
	std::vector<User> const users {{"Fred", 42, {"FC00"}}, {"Jim", 43, {"FD00"}}, {"Sheila", 44, {"FE00"}}};
	Evaluator evaluator;
	evaluator.SetCollection("users", users);

	auto const * xml = R"(<xml xmlns:gl='glib'>
<gl:block each="user : ${users}">
	<User name='${user.name}' />
</gl:block>
</xml>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	auto const * expected = R"(<xml>
	<User name='Fred' />
	<User name='Jim' />
	<User name='Sheila' />
</xml>)";

	TEST(stm.str() == expected);
}

AUTO_TEST_CASE(NestedForEach)
{
	std::vector<User> const users {{"Fred", 42, {"FC00"}}, {"Jim", 43, {"FD00"}}, {"Sheila", 44, {"FE00"}}};

	Evaluator evaluator;
	evaluator.SetCollection("users", users);

	auto const * xml = R"(<xml xmlns:gl='glib'>
<gl:block each="user : ${users}">
	<User name='${user.name}'>
<gl:block each="hobby : ${user.hobbies}">
		<Hobby value='${hobby}'/>
</gl:block>
	</User>
</gl:block>
</xml>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	auto const * expected = R"(<xml>
	<User name='Fred'>
		<Hobby value='FC00'/>
	</User>
	<User name='Jim'>
		<Hobby value='FD00'/>
	</User>
	<User name='Sheila'>
		<Hobby value='FE00'/>
	</User>
</xml>)";

	TEST(stm.str() == expected);
}

AUTO_TEST_CASE(ForEachAttr)
{
	std::vector<User> const users {{"Fred", 42, {"FC00"}}, {"Jim", 43, {"FD00"}}, {"Sheila", 44, {"FE00"}}};
	Evaluator evaluator;
	evaluator.SetCollection("users", users);

	auto const * xml = R"(<xml xmlns:gl='glib'>
<User gl:each="user : ${users}" name='${user.name}'/>
</xml>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	auto const * expected = R"(<xml>
<User name='Fred'/>
<User name='Jim'/>
<User name='Sheila'/>
</xml>)";

	TEST(stm.str() == expected);

	xml = R"(<xml xmlns:gl='glib'>
<User gl:if="${optional}" gl:each="user : ${users}" name='${user.name}'>${user.age}</User>
</xml>)";

	stm.str({});
	evaluator.Set("optional", true);
	Generate(evaluator, xml, stm);

	expected = R"(<xml>
<User name='Fred'>42</User>
<User name='Jim'>43</User>
<User name='Sheila'>44</User>
</xml>)";

	TEST(stm.str() == expected);

	stm.str({});
	evaluator.Set("optional", false);
	Generate(evaluator, xml, stm);

	expected = R"(<xml>
</xml>)";

	TEST(stm.str() == expected);
}

AUTO_TEST_CASE(IfAttr)
{
	Evaluator evaluator;

	auto const * xml = R"(<xml xmlns:gl='glib'>
<User gl:if="false"/>
</xml>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	auto const * expected = R"(<xml>
</xml>)";

	TEST(stm.str() == expected);
}

AUTO_TEST_CASE(ReplaceAttribute)
{
	Evaluator evaluator;

	auto const * xml = "<xml xmlns:gl='glib' attr='value' gl:attr='replacedValue'/>";
	auto const * expected = R"(<xml attr='replacedValue'/>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	TEST(stm.str() == expected);
}

AUTO_TEST_CASE(ReplaceAttributeKeepsOtherXmlNs)
{
	Evaluator evaluator;

	auto const * xml = "<xml xmlns:gl='glib' xmlns:foo='bar' attr='value' gl:attr='replacedValue'/>";
	auto const * expected = R"(<xml xmlns:foo='bar' attr='replacedValue'/>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	TEST(stm.str() == expected);
}

AUTO_TEST_CASE(ReplaceText)
{
	Evaluator evaluator;

	auto const * xml = "<xml xmlns:gl='glib' gl:text='new'>old</xml>";
	auto const * exp = "<xml>new</xml>";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	TEST(stm.str() == exp);
}

AUTO_TEST_CASE(ReplaceTextAndAttr)
{
	Evaluator evaluator;

	auto const * xml = R"(<xml xmlns:gl='glib' gl:attr='newAttr' gl:text='new' attr='oldAttr'>old</xml>)";
	auto const * exp = R"(<xml attr='newAttr'>new</xml>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	TEST(stm.str() == exp);
}

AUTO_TEST_CASE(BugReplaceLosesWhiteSpace)
{
	Evaluator evaluator;

	auto const * xml = R"(<xml xmlns:gl='glib' gl:text='new'>
old
</xml>)";

	auto const * exp = R"(<xml>new</xml>)";

	std::ostringstream stm;
	Generate(evaluator, xml, stm);

	TEST(stm.str() == exp);
}

AUTO_TEST_CASE(If)
{
	auto const * xml = R"(<xml xmlns:gl='glib'>
<gl:block if='${value}'>
	<td>In Block</td>
</gl:block>
</xml>)";

	auto const * expectedYes = R"(<xml>
	<td>In Block</td>
</xml>)";

	auto const * expectedNo = R"(<xml>
</xml>)";

	std::ostringstream stm;
	Evaluator evaluator;

	evaluator.Set("value", true);
	Generate(evaluator, xml, stm);
	TEST(stm.str() == expectedYes);

	stm.str("");
	evaluator.Remove("value");
	evaluator.Set("value", false);
	Generate(evaluator, xml, stm);
	TEST(stm.str() == expectedNo);
}

AUTO_TEST_CASE(IfEach)
{
	auto const * xml = R"(<xml xmlns:gl='glib'>
<gl:block if='${value}' each='var : ${vars}'>
	<td>${var}</td>
</gl:block>
</xml>)";

	auto const * expectedYes = R"(<xml>
	<td>1</td>
	<td>2</td>
	<td>3</td>
</xml>)";

	auto const * expectedNo = R"(<xml>
</xml>)";

	std::ostringstream stm;
	Evaluator evaluator;

	std::vector const vars {1, 2, 3};
	evaluator.SetCollection("vars", vars);
	evaluator.Set("value", true);
	Generate(evaluator, xml, stm);
	TEST(stm.str() == expectedYes);

	stm.str("");
	evaluator.Remove("value");
	evaluator.Set("value", false);
	Generate(evaluator, xml, stm);
	TEST(stm.str() == expectedNo);
}

AUTO_TEST_CASE(TestUtilsTest1) // move
{
	std::ostringstream s;
	CHECK(false == TestUtils::CompareStrings("1234", "1234", 30, s));
}

AUTO_TEST_CASE(TestUtilsTest2)
{
	std::ostringstream s;
	CHECK(true == TestUtils::CompareStrings("12345", "1234", 30, s));
	TEST("Expected data at end missing: [5]" == s.str());
}

AUTO_TEST_CASE(TestUtilsTest3)
{
	std::ostringstream s;
	CHECK(true == TestUtils::CompareStrings("1234", "12345", 30, s));

	TEST(R"(Difference at position: 4
Expected: []
Actual  : [5])" == s.str());
}

AUTO_TEST_CASE(TestUtilsTest4)
{
	std::ostringstream s;
	CHECK(true == TestUtils::CompareStrings("\t\n ", "123", 30, s));

	TEST(R"(Difference at position: 0
Expected: [\t\n\s]
Actual  : [123])" == s.str());
}

AUTO_TEST_CASE(TestUtilsTest5)
{
	GLIB_CHECK_RUNTIME_EXCEPTION({ TestUtils::Compare("1234", "12345", 30); },
															 R"(Difference at position: 4
Expected: []
Actual  : [5])");
}

AUTO_TEST_SUITE_END()