
#include <GLib/Eval/Evaluator.h>

#include <boost/test/unit_test.hpp>

#include "TestStructs.h"
#include "TestUtils.h"

BOOST_AUTO_TEST_SUITE(EvaluatorTests)

BOOST_AUTO_TEST_CASE(AddEnum)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("feefee", Quatrain::Fee);
	evaluator.Set("f00f00", Quatrain::Fo);
	BOOST_TEST("Fee" == evaluator.Evaluate("feefee"));
	BOOST_TEST("Fo" == evaluator.Evaluate("f00f00"));
}

BOOST_AUTO_TEST_CASE(AddStruct)
{
	GLib::Eval::Evaluator evaluator;
	User user {"Zardoz", 999, {}};
	evaluator.Set("user", user);

	std::string name = evaluator.Evaluate("user.name");
	BOOST_TEST(name == "Zardoz");
	std::string age = evaluator.Evaluate("user.age");
	BOOST_TEST(age == "999");
}

BOOST_AUTO_TEST_CASE(NestedStruct)
{
	GLib::Eval::Evaluator evaluator;
	Struct Struct {{"NestedValue"}};
	evaluator.Set("struct", Struct);
	std::string value = evaluator.Evaluate("struct.Nested");
	BOOST_TEST(value == "NestedValue");
}

BOOST_AUTO_TEST_CASE(StructForEach)
{
	GLib::Eval::Evaluator evaluator;

	const std::vector<User> users {{"Fred", 42, {"FC00"}}, {"Jim", 43, {"FD00"}}, {"Sheila", 44, {"FE00"}}};
	evaluator.SetCollection("users", users);

	std::vector<std::string> result;
	evaluator.ForEach("users",
										[&](const GLib::Eval::ValueBase & user)
										{
											std::ostringstream s;
											user.VisitProperty("name", [&](const GLib::Eval::ValueBase & value) { s << value.ToString(); });
											user.VisitProperty("age", [&](const GLib::Eval::ValueBase & value) { s << ':' << value.ToString(); });
											result.push_back(s.str());
										});

	const std::vector<std::string> expected {"Fred:42", "Jim:43", "Sheila:44"};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
}

BOOST_AUTO_TEST_CASE(NestedPropertyForEach)
{
	GLib::Eval::Evaluator evaluator;

	User user {"Fred", 42, {"Computing", "Busses"}};
	evaluator.Set("user", user);

	std::ostringstream s;
	evaluator.ForEach("user.hobbies", [&](const GLib::Eval::ValueBase & value) { s << value.ToString() << ','; });

	BOOST_TEST(s.str() == "Computing,Busses,");
}

BOOST_AUTO_TEST_CASE(NativeTypeForEach)
{
	GLib::Eval::Evaluator evaluator;

	const std::vector integers {1, 2, 3};
	evaluator.SetCollection("integers", integers);

	std::vector<std::string> result;
	evaluator.ForEach("integers", [&](const GLib::Eval::ValueBase & value) { result.push_back(value.ToString()); });

	const std::vector<std::string> expected {"1", "2", "3"};
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
	evaluator.Set("stringVal", "Hello");
	BOOST_TEST("Hello" == evaluator.Evaluate("stringVal"));
	evaluator.Remove("stringVal");
	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("stringVal")); }, "Value not found : stringVal");
}

BOOST_AUTO_TEST_CASE(RemoveNonexistentCollectionThrows)
{
	GLib::Eval::Evaluator evaluator;
	GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.Remove("nonexistent"); }, "Value not found : nonexistent");
}

BOOST_AUTO_TEST_CASE(ResetValue)
{
	User z {"Zardoz", 999, {"Domination", "MassiveHead"}};
	User d {"Diablo", 666, {"Fire", "Brimstone"}};
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("user", z);
	BOOST_TEST("Zardoz" == evaluator.Evaluate("user.name"));
	evaluator.Set("user", d);
	BOOST_TEST("Diablo" == evaluator.Evaluate("user.name"));
}

BOOST_AUTO_TEST_CASE(ResetCollection)
{
	GLib::Eval::Evaluator evaluator;
	std::list values {1, 2, 3};
	evaluator.SetCollection("values", values);
	BOOST_TEST("1,2,3" == evaluator.Evaluate("values")); // delim?

	evaluator.SetCollection("values", values = {4, 5, 6});
	BOOST_TEST("4,5,6" == evaluator.Evaluate("values"));
}

BOOST_AUTO_TEST_CASE(CollectionUnimplementedMethods)
{
	GLib::Eval::Evaluator evaluator;
	const std::vector values {1, 2, 3};
	evaluator.SetCollection("value", values);

	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("value.property")); }, "Not implemented");
}

BOOST_AUTO_TEST_CASE(ValueForEachThrows)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("value", 1234);

	GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.ForEach("value", [&](const GLib::Eval::ValueBase &) {}); }, "ForEach not defined for : int");
}

BOOST_AUTO_TEST_CASE(CollectionForEach)
{
	GLib::Eval::Evaluator evaluator;
	std::vector ints {1, 2, 3, 4};
	evaluator.Set("ints", ints);

	std::vector<std::string> result;
	evaluator.ForEach("ints", [&](const GLib::Eval::ValueBase & value) { result.push_back(value.ToString()); });
	std::vector<std::string> expected {"1", "2", "3", "4"};
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
}

BOOST_AUTO_TEST_CASE(UnknownValueThrows)
{
	GLib::Eval::Evaluator evaluator;
	User user {"Zardoz", 999, {"Domination", "MassiveHead"}};
	evaluator.Set("user", user);

	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("user.HasProperty")); }, "Unknown property : 'HasProperty'");
}

BOOST_AUTO_TEST_CASE(UnknownVisitorThrows)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("HasNoVisitor", HasNoVisitor {});
	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("HasNoVisitor.FuBar")); },
															 "No accessor defined for property: 'FuBar', type:'HasNoVisitor'");
	;
}

BOOST_AUTO_TEST_CASE(NoToStringThrows)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("HasNoToString", HasNoToString {});
	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("HasNoToString")); }, "Cannot convert type to string : HasNoToString");
}

BOOST_AUTO_TEST_CASE(Bool)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("valueTrue", true);
	evaluator.Set("valueFalse", false);
	BOOST_TEST("true" == evaluator.Evaluate("valueTrue"));
	BOOST_TEST("false" == evaluator.Evaluate("valueFalse"));
}

BOOST_AUTO_TEST_SUITE_END()