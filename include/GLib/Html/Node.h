#pragma once

#include <list>
#include <string_view>

namespace GLib::Html
{
	class Node
	{
		Node * const parent {};
		std::string_view value;
		std::string variable;
		std::string enumeration;
		std::list<Node> children; // use ostream for xml fragmemts, single optional child for the rest, polymorphic?

	public:
		Node() = default;

		Node(Node * parent, std::string_view value) : parent(parent), value(value)
		{}

		Node(Node * parent, std::string variable, std::string enumeration)
			: parent(parent), variable(move(variable)), enumeration(move(enumeration))
		{}

		Node * Parent() const
		{
			return parent;
		}

		const std::string_view & Value() const
		{
			return value;
		}

		const std::string & Variable() const
		{
			return variable;
		}

		const std::string & Enumeration() const
		{
			return enumeration;
		}

		const std::list<Node> & Children() const
		{
			return children;
		}

		Node * AddFragment(const std::string_view & fragment={})
		{
			children.emplace_back(this, fragment);
			return &children.back();
		}

		Node * AddEnumeration(const std::string & var, const std::string & e)
		{
			children.emplace_back(this, var, e);
			return &children.back();
		}
	};
}