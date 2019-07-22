
#include <boost/test/unit_test.hpp>

#include "GLib/Eval/TemplateEngine.h"

#include "TestStructs.h"
#include "TestUtils.h"

struct Directory
{
	std::string name;
	std::string trafficLight;
	unsigned int coveragePercent;
	unsigned int lines;
	unsigned int coveredLines;
};

template <>
struct GLib::Eval::Visitor<Directory>
{
	static void Visit(const Directory & dir, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "name") return f(Value(dir.name));
		if (propertyName == "trafficLight") return f(Value(dir.trafficLight));
		if (propertyName == "coveragePercent") return f(Value(dir.coveragePercent));
		if (propertyName == "lines") return f(Value(dir.lines));
		if (propertyName == "coveredLines") return f(Value(dir.coveredLines));
		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\''); // bool return?
	}
};

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
		auto expected = R"(<xml attr="replacedValue" />)";

		std::ostringstream stm;
		TemplateEngine::Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(ReplaceAttributeKeepsOtherXmlNs)
	{
		Evaluator evaluator;

		auto xml = "<xml xmlns:gl='glib' xmlns:foo='bar' attr='value' gl:attr='replacedValue'/>";
		auto expected = R"(<xml xmlns:foo="bar" attr="replacedValue" />)";

		std::ostringstream stm;
		TemplateEngine::Generate(evaluator, xml, stm);

		BOOST_TEST(stm.str() == expected);
	}

	BOOST_AUTO_TEST_CASE(Diff)
	{
		Diff2("1234", "1234", 30);
		GLIB_CHECK_RUNTIME_EXCEPTION({ Diff2("1234", "12345", 30); }, "Expected data at end missing: [5]");
		GLIB_CHECK_RUNTIME_EXCEPTION({ Diff2("12345", "1234", 30); }, "Difference at position: 4 [5]");
		GLIB_CHECK_RUNTIME_EXCEPTION({ Diff2("\t\n ", "123", 30); }, "Difference at position: 0 [\\t\\n\\s]");
	}

	BOOST_AUTO_TEST_CASE(Cov1)
	{
		Evaluator evaluator;
		evaluator.Add("title", "Tests.exe");
		evaluator.Add("styleSheet", "coverage.css");

		std::vector<Directory> directories =
		{ { "dir1","amber", 78, 278, 219 } };
		evaluator.Add("directories", directories);

		auto xml = R"(<!DOCTYPE html>
<html xmlns:gl='glib'>
 <head>
  <meta charset="UTF-8"/>
  <title>Coverage - ${title}</title>
  <link rel="stylesheet" type="text/css" href="./coverage.css" gl:href="${styleSheet}"/>
 </head>
 <body>
  <hr/>
  <table cellpadding="1" cellspacing="1" border="0" class="centre">
   <tr>
    <td>Directory</td>
    <td>Coverage</td>
    <td>%</td>
    <td>Covered lines</td>
   </tr>
<gl:block each="directory : ${directories}">
   <tr>
    <td>
     <a href="${directory.name}/index.html">${directory.name}</a>
    </td>
    <td>
     <div class="box">
      <div class="${directory.trafficLight}" style="width:${directory.coveragePercent}px;"/>
     </div>
    </td>
    <td>${directory.coveragePercent} %</td>
    <td class="coverageNumber">${directory.coveredLines} / ${directory.lines}</td>
   </tr>
</gl:block>
  </table>
  <hr/>
 </body>
</html>
)";

		auto expected = R"(<!DOCTYPE html>
<html>
 <head>
  <meta charset="UTF-8"/>
  <title>Coverage - Tests.exe</title>
  <link rel="stylesheet" type="text/css" href="coverage.css" />
 </head>
 <body>
  <hr/>
  <table cellpadding="1" cellspacing="1" border="0" class="centre">
   <tr>
    <td>Directory</td>
    <td>Coverage</td>
    <td>%</td>
    <td>Covered lines</td>
   </tr>
   <tr>
    <td>
     <a href="dir1/index.html">dir1</a>
    </td>
    <td>
     <div class="box">
      <div class="amber" style="width:78px;"/>
     </div>
    </td>
    <td>78 %</td>
    <td class="coverageNumber">219 / 278</td>
   </tr>
  </table>
  <hr/>
 </body>
</html>)";

		std::ostringstream stm;
		TemplateEngine::Generate(evaluator, xml, stm);

		Diff2(stm.str(), expected, 30);
	}

BOOST_AUTO_TEST_SUITE_END()