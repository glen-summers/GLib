#pragma once

#include <GLib/Eval/Collection.h>

#include <string>

enum class Quatrain : uint8_t
{
	Fee,
	Fi,
	Fo,
	Fum
};

inline std::ostream & operator<<(std::ostream & s, Quatrain q)
{
	constexpr auto a = std::array<std::string_view, 4> {"Fee", "Fi", "Fo", "Fum"};
	return s << a.at(static_cast<uint8_t>(q));
}

struct Struct
{
	struct NestedStruct
	{
		std::string Value;
	} Nested;
};

struct User
{
	std::string Name;
	uint16_t Age;
	std::list<std::string> Hobbies;
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
			return f(Value(user.Name));
		}
		if (propertyName == "age")
		{
			return f(Value(user.Age));
		}
		if (propertyName == "hobbies")
		{
			return f(MakeCollection(user.Hobbies));
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
			return f(Value(value.Nested.Value));
		}
		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\''); // bool return?
	}
};