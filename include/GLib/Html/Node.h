#pragma once

#include <string>
#include <vector>

namespace GLib::Html
{
	class Node;
	using NodeList = std::vector<Node>;

	class Node
	{
		Node * const parent {};
		std::string_view value;
		std::string variable;
		std::string enumeration;
		std::string_view condition;
		NodeList children; // use ostream for xml fragments, single optional child for the rest, polymorphic?
		size_t const depth {};

	public:
		Node() = default;

		Node(Node * parent, std::string_view const value)
			: parent(parent)
			, value(value)
		{}

		Node(Node * parent, std::string variable, std::string enumeration, std::string_view const condition, size_t const depth)
			: parent(parent)
			, variable(std::move(variable))
			, enumeration(std::move(enumeration))
			, condition(condition)
			, depth(depth)
		{}

		Node(Node * parent, std::string_view const condition, bool const unused)
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

		[[nodiscard]] std::string const & Variable() const
		{
			return variable;
		}

		[[nodiscard]] std::string const & Enumeration() const
		{
			return enumeration;
		}

		[[nodiscard]] std::string_view Condition() const
		{
			return condition;
		}

		[[nodiscard]] NodeList const & Children() const
		{
			return children;
		}

		[[nodiscard]] size_t Depth() const
		{
			return depth;
		}

		void AddFragment(std::string_view const fragment)
		{
			children.emplace_back(this, fragment);
		}

		void AddFragment(std::string_view const start, std::string_view const end)
		{
			AddFragment({start.data(), static_cast<size_t>(end.data() - start.data())});
		}

		void AddEnumeration(std::string const & var, std::string const & childEnumeration, std::string_view const childCondition,
												size_t const enumerationDepth)
		{
			children.emplace_back(this, var, childEnumeration, childCondition, enumerationDepth);
		}

		void AddConditional(std::string_view const childCondition)
		{
			children.emplace_back(this, childCondition, true);
		}

		Node * Back()
		{
			return &children.back();
		}
	};
}