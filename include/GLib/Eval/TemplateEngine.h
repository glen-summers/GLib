#pragma once

#include "GLib/Eval/Evaluator.h"
#include "GLib/Xml/Iterator.h"

#include "GLib/PairHash.h"
#include "GLib/scope.h"

#include <regex>

namespace GLib::Eval::TemplateEngine
{
	namespace Detail
	{
		static constexpr auto NameSpace = std::string_view {"glib"};
		static constexpr auto Block = std::string_view {"block"};
		static constexpr auto Each = std::string_view {"each"};

		using AttributeMap = std::unordered_map<std::pair<std::string_view, std::string_view>, Xml::Attribute, Util::PairHash>;

		class Node
		{
			Node * const parent;
			std::string_view value;
			std::list<Node> children;
			std::string variable;
			std::string enumeration;

		public:
			Node() : parent {}
			{}

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

		class Nodes
		{
			std::regex const varRegex { R"(^(\w+)\s:\s\$\{([\w\.]+)\}$)" };
			Node root;
			Node * current;

		public:
			Nodes(const Xml::Holder & holder) : current(&root)
			{
				for (auto it = holder.begin(), end = holder.end(); it != end; ++it)
				{
					const Xml::Element & e = *it;

					if (e.NameSpace() == NameSpace && e.Name() == Block)
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

								current = current->AddEnumeration(m[1], m[2]);
								break;
							}

							case Xml::ElementType::Empty:
							{
								throw std::runtime_error("No block content");
							}

							case Xml::ElementType::Close:
							{
								if (current == nullptr)
								{
									throw std::logic_error("No parent node");
								}
								current = current->Parent();
								break;
							}

							default:
							{
								throw std::logic_error("Unexpected enumeration value");
							}
						}
					}
					else if (e.Type() != Xml::ElementType::Close)
					{
						AttributeMap atMap;
						bool replaced = false;
						const auto & manager = it.Manager();

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
							current->AddFragment(Xml::Utils::ToStringView(e.OuterXml().data(), e.Attributes().Value().data()));

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
									current->AddFragment(a.name);
									current->AddFragment("=\"");
									current->AddFragment(a.value);
									current->AddFragment("\" ");
								}
								else
								{
									auto [name, nameSpace] = manager.Normalise(a.name);
									if (nameSpace.empty())
									{
										// preserve quote type?
										current->AddFragment(name);
										current->AddFragment("=\"");
										current->AddFragment(atMap[std::make_pair(nameSpace, name)].value);
										current->AddFragment("\" ");
									}
								}
							}
							current->AddFragment(Xml::Utils::ToStringView(e.Attributes().Value().data()+e.Attributes().Value().size(), e.OuterXml().data()+e.OuterXml().size()));
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
										current->AddFragment(Xml::Utils::ToStringView(p, a.name.data()-1)); // -1 minus space prefix
										p = a.value.data() + a.value.size() + 1; // +1 trailing quote
									}
								}
							}
							current->AddFragment(Xml::Utils::ToStringView(p, e.OuterXml().data()+e.OuterXml().size()));
						}
					}
					else
					{
						current->AddFragment(e.OuterXml());
					}
				}
			}

			const Node & GetRoot() const
			{
				return root;
			}
		};

		class Generator
		{
			std::regex const propRegex { R"(\$\{([\w\.]+)\})" };

			Evaluator & evaluator;

		public:
			Generator(Evaluator & evaluator)
				: evaluator(evaluator)
			{}

			void Generate(const std::string_view & xml, std::ostream & out)
			{
				Detail::Nodes nodes{xml};
				Generate(nodes.GetRoot(), out);
			}

		private:
			void Generate(const Node & node, std::ostream & out)
			{
				if (!node.Enumeration().empty())
				{
					evaluator.ForEach(node.Enumeration(), [&](const ValueBase & value)
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
	}

	inline void Generate(Evaluator & e, const std::string_view & xml, std::ostream & out)
	{
		Detail::Generator(e).Generate(xml, out);
	}
}