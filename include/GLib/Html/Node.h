#pragma once

#include <list>
#include <string>
#include <string_view>

namespace GLib::Html
{
	class Node
	{
		Node * const parent {};
		std::string_view value;
		std::string variable;
		std::string enumeration;
		std::string_view condition;
		std::list<Node> children; // use ostream for xml fragments, single optional child for the rest, polymorphic?
		size_t const depth {};

	public:
		Node() = default;

		Node(Node * parent, std::string_view value)
			: parent(parent)
			, value(value)
		{}

		Node(Node * parent, std::string variable, std::string enumeration, std::string_view condition, size_t depth)
			: parent(parent)
			, variable(move(variable))
			, enumeration(move(enumeration))
			, condition(condition)
			, depth(depth)
		{}

		Node(Node * parent, std::string_view condition, bool unused)
			: parent(parent)
			, condition(condition)
		{
			static_cast<void>(unused);
		}

		[[nodiscard]] Node * Parent() const
		{
			return parent;
		}

		[[nodiscard]] std::string_view Value() const
		{
			return value;
		}

		[[nodiscard]] const std::string & Variable() const
		{
			return variable;
		}

		[[nodiscard]] const std::string & Enumeration() const
		{
			return enumeration;
		}

		[[nodiscard]] std::string_view Condition() const
		{
			return condition;
		}

		[[nodiscard]] const std::list<Node> & Children() const
		{
			return children;
		}

		[[nodiscard]] size_t Depth() const
		{
			return depth;
		}

		Node * AddFragment(std::string_view fragment = {})
		{
			children.emplace_back(this, fragment);
			return &children.back();
		}

		Node * AddFragment(const char * start, const char * end)
		{
			return AddFragment({start, static_cast<size_t>(end - start)});
		}

		Node * AddEnumeration(const std::string & var, const std::string & e, std::string_view c, size_t enumDepth)
		{
			children.emplace_back(this, var, e, c, enumDepth);
			return &children.back();
		}

		Node * AddConditional(std::string_view c)
		{
			children.emplace_back(this, c, true);
			return &children.back();
		}
	};
}