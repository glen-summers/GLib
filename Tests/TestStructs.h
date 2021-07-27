#pragma once

#include <GLib/Eval/Collection.h>

#include <string>

enum class Quatrain : int
{
	Fee,
	Fi,
	Fo,
	Fum
};

inline std::ostream & operator<<(std::ostream & s, Quatrain q)
{
	constexpr auto a = std::array<std::string_view, 4> {"Fee", "Fi", "Fo", "Fum"};
	return s << a.at(static_cast<int>(q));
}

struct Struct
{
	struct NestedStruct
	{
		std::string value;
	} Nested;
};

struct User
{
	std::string name;
	int age;
	std::list<std::string> hobbies;
};

struct HasNoVisitor
{};

struct HasNoToString
{};

template <>
struct GLib::Eval::Visitor<User>
{
	static void Visit(const User & user, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "name")
		{
			return f(Value(user.name));
		}
		if (propertyName == "age")
		{
			return f(Value(user.age));
		}
		if (propertyName == "hobbies")
		{
			return f(MakeCollection(user.hobbies));
		}
		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\''); // bool return?
	}
};

template <>
struct GLib::Eval::Visitor<Struct>
{
	static void Visit(const Struct & value, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "Nested")
		{
			return f(Value(value.Nested.value));
		}
		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\''); // bool return?
	}
};