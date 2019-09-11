#pragma once

#include "Node.h"

#include "GLib/Eval/Evaluator.h"
#include "GLib/Xml/Iterator.h"

#include "GLib/PairHash.h"

#include <ostream>
#include <regex>

namespace GLib::Html
{
	class Generator
	{
		using AttributeMap = std::unordered_map<std::pair<std::string_view, std::string_view>, Xml::Attribute, Util::PairHash>;

		static constexpr auto NameSpace = std::string_view {"glib"};
		static constexpr auto Block = std::string_view {"block"};
		static constexpr auto Each = std::string_view {"each"};

		std::regex const propRegex { R"(\$\{([\w\.]+)\})" };
		std::regex const varRegex { R"(^(\w+)\s:\s\$\{([\w\.]+)\}$)" };

		Eval::Evaluator & evaluator;

	public:
		Generator(Eval::Evaluator & evaluator)
			: evaluator(evaluator)
		{}

		void Generate(const std::string_view & xml, std::ostream & out)
		{
			Generate(Parse(xml), out);
		}

	private:
		Node Parse(const std::string_view xml)
		{
			Node root;
			Node * current = &root;

			Xml::Holder holder{xml};
			for (auto it = holder.begin(), end = holder.end(); it != end; ++it)
			{
				const Xml::Element & e = *it;

				if (e.NameSpace() == NameSpace && e.Name() == Block)
				{
					current = ProcessBlock(e, current);
				}
				else if (e.Type() != Xml::ElementType::Close)
				{
					current = ProcessElement(e, current, holder.Manager());
				}
				else
				{
					current->AddFragment(e.OuterXml());
				}
			}

			return root;
		}

		Node * ProcessBlock(const Xml::Element & e, Node * node)
		{
			switch (e.Type())
			{
				case Xml::ElementType::Open:
				{
					auto eachIt = e.Attributes().begin();
					if (eachIt == e.Attributes().end() || (*eachIt).name != Each)
					{
						throw std::runtime_error("No each attribute");
					}

					const std::string_view & var = (*eachIt).value;
					std::match_results<std::string_view::const_iterator> m;
					std::regex_search(var.begin(), var.end(), m, varRegex);
					if (m.empty())
					{
						throw std::runtime_error("Error in var : " + std::string(var));
					}

					return node->AddEnumeration(m[1], m[2]);
				}

				case Xml::ElementType::Empty:
				{
					throw std::runtime_error("No block content");
				}

				case Xml::ElementType::Close:
				{
					if (node == nullptr)
					{
						throw std::logic_error("No parent node");
					}
					return node->Parent();
				}

				default:
				{
					throw std::logic_error("Unexpected enumeration value");
				}
			}
		}

		Node * ProcessElement(const Xml::Element & e, Node * node, const Xml::NameSpaceManager & manager)
		{
			AttributeMap atMap;
			bool replaced = false;

			Xml::Attributes attributes { e.Attributes().Value(), nullptr };
			for (auto a : attributes)
			{
				if (!Xml::NameSpaceManager::IsDeclaration(a.name))
				{
					auto [name, nameSpace] = manager.Normalise(a.name);
					if (nameSpace.empty())
					{
						atMap.emplace(std::make_pair(nameSpace, name), a);
					}
					else if (nameSpace == NameSpace)
					{
						auto existingIt = atMap.find(std::make_pair("",name));
						if (existingIt == atMap.end())
						{
							throw std::runtime_error(std::string("Attribute not found : '") + std::string(name) + "'");
						}
						existingIt->second.value = a.value;
						replaced = true;
					}
				}
			}

			if (replaced)
			{
				node->AddFragment(Xml::Utils::ToStringView(e.OuterXml().data(), e.Attributes().Value().data()));

				for (const auto & a : attributes)
				{
					std::string_view nameSpacePrefix;
					if (Xml::NameSpaceManager::CheckForDeclaration(a.name, nameSpacePrefix))
					{
						auto ns = manager.Get(nameSpacePrefix);
						if (ns == NameSpace)
						{
							continue;
						}
						// preserve quote type?
						// could add entire attribute text, would need initial space as part of value
						// could also accumulate and add single fragment
						node->AddFragment(a.name);
						node->AddFragment("=\"");
						node->AddFragment(a.value);
						node->AddFragment("\" ");
					}
					else
					{
						auto [name, nameSpace] = manager.Normalise(a.name);
						if (nameSpace.empty())
						{
							// preserve quote type?
							node->AddFragment(name);
							node->AddFragment("=\"");
							node->AddFragment(atMap[std::make_pair(nameSpace, name)].value);
							node->AddFragment("\" ");
						}
					}
				}
				node->AddFragment(Xml::Utils::ToStringView(e.Attributes().Value().data()+e.Attributes().Value().size(), e.OuterXml().data()+e.OuterXml().size()));
			}
			else
			{
				auto p = e.OuterXml().data();
				for (const Xml::Attribute & a : attributes)
				{
					std::string_view prefix;
					if (Xml::NameSpaceManager::CheckForDeclaration(a.name, prefix))
					{
						if (manager.Get(prefix) == NameSpace)
						{
							node->AddFragment(Xml::Utils::ToStringView(p, a.name.data()-1)); // -1 minus space prefix
							p = a.value.data() + a.value.size() + 1; // +1 trailing quote
						}
					}
				}
				node->AddFragment(Xml::Utils::ToStringView(p, e.OuterXml().data()+e.OuterXml().size()));
			}
			return node;
		}

		void Generate(const Node & node, std::ostream & out)
		{
			if (!node.Enumeration().empty())
			{
				evaluator.ForEach(node.Enumeration(), [&](const Eval::ValueBase & value)
				{
					evaluator.Push(node.Variable(), value);

					for (const auto & child : node.Children())
					{
						Generate(child, out);
					}

					evaluator.Pop(node.Variable());
				});
				return;
			}

			if (!node.Value().empty())
			{
				std::regex r(propRegex);
				std::cregex_iterator it(node.Value().data(), node.Value().data() + node.Value().size(), r);
				auto end = std::cregex_iterator{};
				if (it==end)
				{
					out << node.Value();
				}
				else
				{
					for (;;)
					{
						out << it->prefix();
						auto var = (*it)[1]; // +format;
						out << evaluator.Evaluate(var);
						auto suffix = it->suffix();
						if (++it == end)
						{
							out << suffix;
							break;
						}
					}
				}
			}

			for (const auto & child : node.Children())
			{
				Generate(child, out);
			}
		}
	};

	inline void Generate(GLib::Eval::Evaluator & e, const std::string_view & xml, std::ostream & out)
	{
		Generator(e).Generate(xml, out);
	}
}