
#include <boost/test/unit_test.hpp>

#include "GLib/Eval/Evaluator.h"

#include "TestStructs.h"
#include "TestUtils.h"

BOOST_AUTO_TEST_SUITE(EvaluatorTests)

	BOOST_AUTO_TEST_CASE(AddStruct)
	{
		GLib::Eval::Evaluator evaluator;
		User user {"Zardoz", 999, {}};
		evaluator.Add("user", user);

		std::string name = evaluator.Evaluate("user.name");
		BOOST_TEST(name == "Zardoz");
		std::string age = evaluator.Evaluate("user.age");
		BOOST_TEST(age == "999");
	}

	BOOST_AUTO_TEST_CASE(NestedStruct)
	{
		GLib::Eval::Evaluator evaluator;
		Struct Struct { { "NestedValue" }};
		evaluator.Add("struct", Struct);
		std::string value = evaluator.Evaluate("struct.Nested");
		BOOST_TEST(value == "NestedValue");
	}

	BOOST_AUTO_TEST_CASE(StructForEach)
	{
		GLib::Eval::Evaluator evaluator;

		const std::vector<User> users
		{
			{ "Fred", 42, { "FC00"} }, { "Jim", 43, { "FD00"} }, { "Sheila", 44, { "FE00"} }
		};
		evaluator.AddCollection("users", users);

		std::vector<std::string> result;
		evaluator.ForEach("users", [&](const GLib::Eval::ValueBase & user)
		{
			std::ostringstream s;
			user.VisitProperty("name", [&](const GLib::Eval::ValueBase& value){ s << value.ToString(); });
			user.VisitProperty("age", [&](const GLib::Eval::ValueBase& value){ s << ':' << value.ToString(); });
			result.push_back(s.str());
		});

		const std::vector<std::string> expected { "Fred:42", "Jim:43", "Sheila:44" };
		BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
	}

	BOOST_AUTO_TEST_CASE(NestedPropertyForEach)
	{
		GLib::Eval::Evaluator evaluator;

		User user { "Fred", 42, { "Computing", "Busses" } };
		evaluator.Add("user", user);

		std::vector<std::string> result;
		std::ostringstream s;
		evaluator.ForEach("user.hobbies", [&](const GLib::Eval::ValueBase & value)
		{
			s << value.ToString() << ',';
		});

		BOOST_TEST(s.str() == "Computing,Busses,");
	}

	BOOST_AUTO_TEST_CASE(NativeTypeForEach)
	{
		GLib::Eval::Evaluator evaluator;

		const std::vector<int> integers { 1, 2, 3 };
		evaluator.AddCollection("integers", integers);

		std::vector<std::string> result;
		evaluator.ForEach("integers", [&](const GLib::Eval::ValueBase & value) { result.push_back(value.ToString()); });

		const std::vector<std::string> expected { "1", "2", "3" };
		BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
	}

	BOOST_AUTO_TEST_CASE(RemoveNonexistentValueThrows)
	{
		GLib::Eval::Evaluator evaluator;
		GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.Remove("nonexistent"); }, "Value not found : nonexistent");
	}

	BOOST_AUTO_TEST_CASE(RemoveValue)
	{
		GLib::Eval::Evaluator evaluator;
		evaluator.Add("stringVal", "Hello");
		BOOST_TEST("Hello" == evaluator.Evaluate("stringVal"));
		evaluator.Remove("stringVal");
		GLIB_CHECK_RUNTIME_EXCEPTION({ (void)evaluator.Evaluate("stringVal"); }, "Value not found : stringVal");
	}
	
	BOOST_AUTO_TEST_CASE(RemoveNonexistentCollectionThrows)
	{
		GLib::Eval::Evaluator evaluator;
		GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.Remove("nonexistent"); }, "Value not found : nonexistent");
	}

	BOOST_AUTO_TEST_CASE(AddTwiceThrows)
	{
		User user {"Zardoz", 999, {"Domination", "MassiveHead"}};
		GLib::Eval::Evaluator evaluator;
		evaluator.Add("user", user);

		GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.Add("user", user); }, "Value already exists : user");
	}

	BOOST_AUTO_TEST_CASE(AddCollectionTwiceThrows)
	{
		GLib::Eval::Evaluator evaluator;
		std::list<int> values {1,2,3};
		evaluator.AddCollection("values", values);

		GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.AddCollection("values", values); }, "Value already exists : values");
	}

	BOOST_AUTO_TEST_CASE(CollectionUnimplementedMethods)
	{
		GLib::Eval::Evaluator evaluator;
		const std::vector<int> values { 1, 2, 3 };
		evaluator.AddCollection("value", values);

		GLIB_CHECK_RUNTIME_EXCEPTION({ (void)evaluator.Evaluate("value"); }, "Not implemented");
		GLIB_CHECK_RUNTIME_EXCEPTION({ (void)evaluator.Evaluate("value.property"); }, "Not implemented");
	}

	BOOST_AUTO_TEST_CASE(ValueForEachThrows)
	{
		GLib::Eval::Evaluator evaluator;
		evaluator.Add("value", 1234);

		GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.ForEach("value", [&](const GLib::Eval::ValueBase &) { }); },
			"ForEach not defined for : int");
	}

	BOOST_AUTO_TEST_CASE(CollectionForEach)
	{
		GLib::Eval::Evaluator evaluator;
		std::vector<int> ints { 1,2,3,4};
		evaluator.Add("ints", ints);

		std::vector<std::string> result;
		evaluator.ForEach("ints", [&](const GLib::Eval::ValueBase & value)
		{
				result.push_back(value.ToString());
		});
		std::vector<std::string> expected {"1","2","3","4"} ;
		BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
	}

	BOOST_AUTO_TEST_CASE(UnknownValueThrows)
	{
		GLib::Eval::Evaluator evaluator;
		User user {"Zardoz", 999, {"Domination", "MassiveHead"}};
		evaluator.Add("user", user);

		GLIB_CHECK_RUNTIME_EXCEPTION({ (void)evaluator.Evaluate("user.HasProperty"); },
			"Unknown property : 'HasProperty'");
	}

	BOOST_AUTO_TEST_CASE(UnknownVisitorThrows)
	{
		GLib::Eval::Evaluator evaluator;
		evaluator.Add("HasNoVisitor", HasNoVisitor{});
		GLIB_CHECK_RUNTIME_EXCEPTION({ (void)evaluator.Evaluate("HasNoVisitor.FuBar"); },
			"No accessor defined for property: 'FuBar', type:'HasNoVisitor'");
		;
	}

	BOOST_AUTO_TEST_CASE(NoToStringThrows)
	{
		GLib::Eval::Evaluator evaluator;
		evaluator.Add("HasNoToString", HasNoToString{});
		GLIB_CHECK_RUNTIME_EXCEPTION({ (void)evaluator.Evaluate("HasNoToString"); },
			"Cannot convert type to string : HasNoToString");
	}

	BOOST_AUTO_TEST_CASE(Bool)
	{
		GLib::Eval::Evaluator evaluator;
		evaluator.Add("valueTrue", true);
		evaluator.Add("valueFalse", false);
		BOOST_TEST("true" == evaluator.Evaluate("valueTrue"));
		BOOST_TEST("false" == evaluator.Evaluate("valueFalse"));
	}

BOOST_AUTO_TEST_SUITE_END()