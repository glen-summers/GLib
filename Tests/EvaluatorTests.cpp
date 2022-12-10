
#include <GLib/Eval/Evaluator.h>

#include <boost/test/unit_test.hpp>

#include "TestStructs.h"
#include "TestUtils.h"

AUTO_TEST_SUITE(EvaluatorTests)

AUTO_TEST_CASE(AddEnum)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("feefee", Quatrain::Fee);
	evaluator.Set("f00f00", Quatrain::Fo);
	TEST("Fee" == evaluator.Evaluate("feefee"));
	TEST("Fo" == evaluator.Evaluate("f00f00"));
}

AUTO_TEST_CASE(AddStruct)
{
	GLib::Eval::Evaluator evaluator;
	User const user {"Zardoz", U16(999), {}};
	evaluator.Set("user", user);

	std::string const name = evaluator.Evaluate("user.name");
	TEST(name == "Zardoz");
	std::string const ageValue = evaluator.Evaluate("user.age");
	TEST(ageValue == "999");
}

AUTO_TEST_CASE(NestedStruct)
{
	GLib::Eval::Evaluator evaluator;
	Struct const Struct {{"NestedValue"}};
	evaluator.Set("struct", Struct);
	std::string const value = evaluator.Evaluate("struct.Nested");
	TEST(value == "NestedValue");
}

AUTO_TEST_CASE(StructForEach)
{
	GLib::Eval::Evaluator evaluator;

	std::vector<User> const users {{"Fred", U16(42), {"FC00"}}, {"Jim", U16(43), {"FD00"}}, {"Sheila", U16(44), {"FE00"}}};
	evaluator.SetCollection("users", users);

	std::vector<std::string> result;
	evaluator.ForEach("users",
										[&](GLib::Eval::ValueBase const & user)
										{
											std::ostringstream stm;
											user.VisitProperty("name", [&](GLib::Eval::ValueBase const & value) { stm << value.ToString(); });
											user.VisitProperty("age", [&](GLib::Eval::ValueBase const & value) { stm << ':' << value.ToString(); });
											result.push_back(stm.str());
										});

	std::vector<std::string> const expected {"Fred:42", "Jim:43", "Sheila:44"};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
}

AUTO_TEST_CASE(NestedPropertyForEach)
{
	GLib::Eval::Evaluator evaluator;

	User const user {"Fred", U16(42), {"Computing", "Busses"}};
	evaluator.Set("user", user);

	std::ostringstream stm;
	evaluator.ForEach("user.hobbies", [&](GLib::Eval::ValueBase const & value) { stm << value.ToString() << ','; });

	TEST(stm.str() == "Computing,Busses,");
}

AUTO_TEST_CASE(NativeTypeForEach)
{
	GLib::Eval::Evaluator evaluator;

	std::vector const integers {1, 2, 3};
	evaluator.SetCollection("integers", integers);

	std::vector<std::string> result;
	evaluator.ForEach("integers", [&](GLib::Eval::ValueBase const & value) { result.push_back(value.ToString()); });

	std::vector<std::string> const expected {"1", "2", "3"};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
}

AUTO_TEST_CASE(RemoveNonexistentValueThrows)
{
	GLib::Eval::Evaluator evaluator;
	GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.Remove("nonexistent"); }, "Value not found : nonexistent");
}

AUTO_TEST_CASE(RemoveValue)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("stringVal", "Hello");
	TEST("Hello" == evaluator.Evaluate("stringVal"));
	evaluator.Remove("stringVal");
	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("stringVal")); }, "Value not found : stringVal");
}

AUTO_TEST_CASE(RemoveNonexistentCollectionThrows)
{
	GLib::Eval::Evaluator evaluator;
	GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.Remove("nonexistent"); }, "Value not found : nonexistent");
}

AUTO_TEST_CASE(ResetValue)
{
	User const z {"Zardoz", U16(999), {"Domination", "MassiveHead"}};
	User const d {"Diablo", U16(666), {"Fire", "Brimstone"}};

	GLib::Eval::Evaluator evaluator;
	evaluator.Set("user", z);
	TEST("Zardoz" == evaluator.Evaluate("user.name"));
	evaluator.Set("user", d);
	TEST("Diablo" == evaluator.Evaluate("user.name"));
}

AUTO_TEST_CASE(ResetCollection)
{
	GLib::Eval::Evaluator evaluator;
	std::list values {1, 2, 3};
	evaluator.SetCollection("values", values);
	TEST("1,2,3" == evaluator.Evaluate("values")); // delim?

	evaluator.SetCollection("values", values = {I32(4), I32(5), I32(6)});
	TEST("4,5,6" == evaluator.Evaluate("values"));
}

AUTO_TEST_CASE(CollectionUnimplementedMethods)
{
	GLib::Eval::Evaluator evaluator;
	std::vector const values {1, 2, 3};
	evaluator.SetCollection("value", values);

	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("value.property")); }, "Not implemented");
}

AUTO_TEST_CASE(ValueForEachThrows)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("value", I32(1234));

	GLIB_CHECK_RUNTIME_EXCEPTION({ evaluator.ForEach("value", [&](GLib::Eval::ValueBase const &) {}); }, "ForEach not defined for : int");
}

AUTO_TEST_CASE(CollectionForEach)
{
	GLib::Eval::Evaluator evaluator;
	std::vector const ints {1, 2, 3, 4};
	evaluator.Set("ints", ints);

	std::vector<std::string> result;
	evaluator.ForEach("ints", [&](GLib::Eval::ValueBase const & value) { result.push_back(value.ToString()); });
	std::vector<std::string> expected {"1", "2", "3", "4"};
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), result.begin(), result.end());
}

AUTO_TEST_CASE(UnknownValueThrows)
{
	GLib::Eval::Evaluator evaluator;
	User const user {"Zardoz", U16(999), {"Domination", "MassiveHead"}};
	evaluator.Set("user", user);

	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("user.HasProperty")); }, "Unknown property : 'HasProperty'");
}

AUTO_TEST_CASE(UnknownVisitorThrows)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("HasNoVisitor", HasNoVisitor {});
	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("HasNoVisitor.FuBar")); },
															 "No accessor defined for property: 'FuBar', type:'HasNoVisitor'");
}

AUTO_TEST_CASE(NoToStringThrows)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("HasNoToString", HasNoToString {});
	GLIB_CHECK_RUNTIME_EXCEPTION({ static_cast<void>(evaluator.Evaluate("HasNoToString")); }, "Cannot convert type to string : HasNoToString");
}

AUTO_TEST_CASE(Bool)
{
	GLib::Eval::Evaluator evaluator;
	evaluator.Set("valueTrue", true);
	evaluator.Set("valueFalse", false);
	TEST("true" == evaluator.Evaluate("valueTrue"));
	TEST("false" == evaluator.Evaluate("valueFalse"));
}

AUTO_TEST_SUITE_END()