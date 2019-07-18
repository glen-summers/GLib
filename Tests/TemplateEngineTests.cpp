
#include <boost/test/unit_test.hpp>

#include "GLib/Eval/TemplateEngine.h"

#include "TestStructs.h"
#include "TestUtils.h"

using namespace GLib::Eval;

BOOST_AUTO_TEST_SUITE(TemplateEngineTests)

BOOST_AUTO_TEST_CASE(SimpleProperty)
{
	Evaluator evaluator;
	evaluator.Add<std::string>("name", "fred");

	std::ostringstream stm;
	TemplateEngine::Generate(evaluator, "<xml attr='${name}' />", stm);

	BOOST_TEST(stm.str() == "<xml attr='fred' />");
}

	BOOST_AUTO_TEST_CASE(Nop)
	{
		Evaluator evaluator;
		std::ostringstream stm;
		TemplateEngine::Generate(evaluator, "<xml xmlns:gl='glib'/>", stm);
		BOOST_TEST(stm.str() == "<xml/>");
	}

	BOOST_AUTO_TEST_CASE(Nop2)
	{
		Evaluator evaluator;
		std::ostringstream stm;
		TemplateEngine::Generate(evaluator, "<xml xmlns:gl1='glib' xmlns:gl2='glib'/>", stm);
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
		TemplateEngine::Generate(evaluator, xml, stm);

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
		TemplateEngine::Generate(evaluator, xml, stm);

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

	BOOST_AUTO_TEST_CASE(ReplaceAttribute)
	{
		Evaluator evaluator;

		auto xml = "<xml xmlns:gl='glib' attr='value' gl:attr='replacedValue'/>";
		auto expected = "<xml attr='replacedValue' />";

		std::ostringstream stm;
		TemplateEngine::Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(ReplaceAttributeKeepsOtherXmlNs)
	{
		Evaluator evaluator;

		auto xml = "<xml xmlns:gl='glib' xmlns:foo='bar' attr='value' gl:attr='replacedValue'/>";
		auto expected = "<xml xmlns:foo='bar' attr='replacedValue' />";

		std::ostringstream stm;
		TemplateEngine::Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == expected);
	}

BOOST_AUTO_TEST_SUITE_END()