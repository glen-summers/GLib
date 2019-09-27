
#include <boost/test/unit_test.hpp>

#include "GLib/Html/TemplateEngine.h"

#include "TestStructs.h"
#include "TestUtils.h"

using namespace GLib::Eval;
using namespace GLib::Html;

BOOST_AUTO_TEST_SUITE(TemplateEngineTests)

BOOST_AUTO_TEST_CASE(SimpleProperty)
{
	Evaluator evaluator;
	evaluator.Add<std::string>("name", "fred");

	std::ostringstream stm;
	Generate(evaluator, "<xml attr='${name}' />", stm);

	BOOST_TEST(stm.str() == "<xml attr='fred' />");
}

	BOOST_AUTO_TEST_CASE(Nop)
	{
		Evaluator evaluator;
		std::ostringstream stm;
		Generate(evaluator, "<xml xmlns:gl='glib'/>", stm);
		BOOST_TEST(stm.str() == "<xml/>");
	}

	BOOST_AUTO_TEST_CASE(Nop2)
	{
		Evaluator evaluator;
		std::ostringstream stm;
		Generate(evaluator, "<xml xmlns:gl1='glib' xmlns:gl2='glib'/>", stm);
		BOOST_TEST(stm.str() == "<xml/>");
	}

	BOOST_AUTO_TEST_CASE(ForEach)
	{
		const std::vector<User> users
		{
			{ "Fred", 42, { "FC00"} }, { "Jim", 43, { "FD00"} }, { "Sheila", 44, { "FE00"} }
		};
		Evaluator evaluator;
		evaluator.AddCollection("users", users);

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

	BOOST_AUTO_TEST_CASE(NestedForEach)
	{
		const std::vector<User> users
		{
			{ "Fred", 42, { "FC00" } },
			{ "Jim", 43, { "FD00" } },
			{ "Sheila", 44, { "FE00"} }
		};

		Evaluator evaluator;
		evaluator.AddCollection("users", users);

		auto xml = R"(<xml xmlns:gl='glib'>
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

		auto expected= R"(<xml>
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

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(ForEachAttr1)
	{
		const std::vector<User> users
		{
			{ "Fred", 42, { "FC00"} }, { "Jim", 43, { "FD00"} }, { "Sheila", 44, { "FE00"} }
		};
		Evaluator evaluator;
		evaluator.AddCollection("users", users);

		auto xml = R"(<xml xmlns:gl='glib'>
<User gl:each="user : ${users}" name='${user.name}'/>
</xml>)";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

	auto expected= R"(<xml>
<User name='Fred'/>
<User name='Jim'/>
<User name='Sheila'/>
</xml>)";

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(ForEachAttr2)
	{
		const std::vector<User> users
		{
			{ "Fred", 42, { "FC00"} }, { "Jim", 43, { "FD00"} }, { "Sheila", 44, { "FE00"} }
		};
		Evaluator evaluator;
		evaluator.AddCollection("users", users);

		auto xml = R"(<xml xmlns:gl='glib'>
<User gl:each="user : ${users}" name='${user.name}'>${user.age}</User>
</xml>)";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

		auto expected= R"(<xml>
<User name='Fred'>42</User>
<User name='Jim'>43</User>
<User name='Sheila'>44</User>
</xml>)";

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(ReplaceAttribute)
	{
		Evaluator evaluator;

		auto xml = "<xml xmlns:gl='glib' attr='value' gl:attr='replacedValue'/>";
		auto expected = R"(<xml attr='replacedValue'/>)";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(ReplaceAttributeKeepsOtherXmlNs)
	{
		Evaluator evaluator;

		auto xml = "<xml xmlns:gl='glib' xmlns:foo='bar' attr='value' gl:attr='replacedValue'/>";
		auto expected = R"(<xml xmlns:foo='bar' attr='replacedValue'/>)";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(ReplaceText)
	{
		Evaluator evaluator;

		auto xml = "<xml xmlns:gl='glib' gl:text='new'>old</xml>";
		auto exp = "<xml>new</xml>";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == exp);
	}

	BOOST_AUTO_TEST_CASE(ReplaceTextAndAttr)
	{
		Evaluator evaluator;

		auto xml = R"(<xml xmlns:gl='glib' gl:attr='newAttr' gl:text='new' attr='oldAttr'>old</xml>)";
		auto exp = R"(<xml attr='newAttr'>new</xml>)";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == exp);
	}

	BOOST_AUTO_TEST_CASE(BugReplaceLosesWhiteSpace)
	{
		Evaluator evaluator;

		auto xml = R"(<xml xmlns:gl='glib' gl:text='new'>
old
</xml>)";

		auto exp = R"(<xml>new</xml>)";

		std::ostringstream stm;
		Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == exp);
	}

	BOOST_AUTO_TEST_CASE(If)
	{
		auto xml = R"(<xml xmlns:gl='glib'>
<gl:block if='${value}'>
	<td>In Block</td>
</gl:block>
</xml>)";

		auto expectedYes = R"(<xml>
	<td>In Block</td>
</xml>)";

		auto expectedNo = R"(<xml>
</xml>)";

		std::ostringstream stm;
		Evaluator evaluator;

		evaluator.Add("value", true);
		Generate(evaluator, xml, stm);
		BOOST_TEST(stm.str() == expectedYes);

		stm.str("");
		evaluator.Remove("value");
		evaluator.Add("value", false);
		Generate(evaluator, xml, stm);
		BOOST_TEST(stm.str() == expectedNo);
	}

	BOOST_AUTO_TEST_CASE(IfEach)
	{
		auto xml = R"(<xml xmlns:gl='glib'>
<gl:block if='${value}' each='var : ${vars}'>
	<td>${var}</td>
</gl:block>
</xml>)";

		auto expectedYes = R"(<xml>
	<td>1</td>
	<td>2</td>
	<td>3</td>
</xml>)";

		auto expectedNo= R"(<xml>
</xml>)";

		std::ostringstream stm;
		Evaluator evaluator;

		std::vector<int> vars{1,2,3};
		evaluator.AddCollection("vars", vars);
		evaluator.Add("value", true);
		Generate(evaluator, xml, stm);
		BOOST_TEST(stm.str() == expectedYes);

		stm.str("");
		evaluator.Remove("value");
		evaluator.Add("value", false);
		Generate(evaluator, xml, stm);
		BOOST_TEST(stm.str() == expectedNo);
	}

	BOOST_AUTO_TEST_CASE(TestUtilsTest1) // move
	{
		std::ostringstream s;
		BOOST_CHECK(false == TestUtils::CompareStrings("1234", "1234", 30, s));
	}

	BOOST_AUTO_TEST_CASE(TestUtilsTest2)
	{
		std::ostringstream s;
		BOOST_CHECK(true == TestUtils::CompareStrings("12345", "1234", 30, s));
		BOOST_TEST("Expected data at end missing: [5]" == s.str());
	}

	BOOST_AUTO_TEST_CASE(TestUtilsTest3)
	{
		std::ostringstream s;
		BOOST_CHECK(true == TestUtils::CompareStrings("1234", "12345", 30, s));

		BOOST_TEST(R"(Difference at position: 4
Expected: []
Actual  : [5])" == s.str());
	}

	BOOST_AUTO_TEST_CASE(TestUtilsTest4)
	{
		std::ostringstream s;
		BOOST_CHECK(true == TestUtils::CompareStrings("\t\n ", "123", 30, s));

		BOOST_TEST(R"(Difference at position: 0
Expected: [\t\n\s]
Actual  : [123])" == s.str());
	}

	BOOST_AUTO_TEST_CASE(TestUtilsTest5)
	{
		GLIB_CHECK_RUNTIME_EXCEPTION({ TestUtils::Compare("1234", "12345", 30); },
			R"(Difference at position: 4
Expected: []
Actual  : [5])");
	}

BOOST_AUTO_TEST_SUITE_END()