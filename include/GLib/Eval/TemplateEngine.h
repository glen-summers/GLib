#pragma once

#include "GLib/Eval/Evaluator.h"

#include "GLib/Xml/Iterator.h"
#include "GLib/scope.h"

#include <regex>

namespace GLib::Eval::TemplateEngine
{
	namespace Detail
	{
		inline static std::regex const varRegex { R"(^(\w+)\s:\s\$\{([\w\.]+)\}$)" };
		inline static std::regex const propRegex{R"(\$\{([\w\.]+)\})"};

		struct Node
		{
			Node * const parent;
			std::string_view value;
			std::list<Node> children;
			std::string variable;
			std::string  enumeration;

			Node(Node * parent={}, std::string_view value={}) : parent(parent), value(value)
			{}

			void AddChild(std::string_view v={})
			{
				children.push_back({this, v});
			}
		};

		class Nodes
		{
			Node root;
			Node * current;

		public:
			Nodes(const std::string_view & xml) : current(&root)
			{
				Xml::Holder h{xml};
				auto contentStart = xml.data();
				auto contentEnd = contentStart + xml.size();
		
				auto it = h.begin();
				auto end = h.end();

				// avoid, need to suppress ns, need to change xml test scan to find glib ns and remove
				it.AddNameSpace("gl", "glib"); 

				for (; it != end; ++it)
				{
					if (it->nameSpace=="glib" && it->name=="block")
					{
						auto elementStart = it->outerXml.data();

						switch (it->type)
						{
							case Xml::ElementType::Open:
							{
								if (it->attributes.size()!=1 && it->attributes[0].name != "each")
								{
									throw std::runtime_error("No each attribute");
								}
								auto var = static_cast<std::string>(it->attributes[0].value); // avoid copy to string
								std::smatch m;
								std::regex_search(var, m, varRegex);
								if (m.empty())
								{
									throw std::runtime_error("Error in var : " + var);
								}

								AddXmlFragment(contentStart, elementStart);
								current->AddChild();
								current = &current->children.back();

								current->variable = m[1];
								current->enumeration = m[2];
								contentStart = elementStart + it->outerXml.size();
								break;
							}

							case Xml::ElementType::Empty:
							{
								throw std::runtime_error("No block content");
							}

							case Xml::ElementType::Close:
							{
								AddXmlFragment(contentStart, elementStart);
								if (!current)
								{
									throw std::logic_error("No parent node");
								}
								current = current->parent;
								contentStart = elementStart + it->outerXml.size();
								break;
							}

							default:
							{
								throw std::logic_error("Unexpected enumeration value");
							}
						}
					}
				}
				current->AddChild({ contentStart, static_cast<size_t>(contentEnd - contentStart) });
			}

			const Node & GetRoot() const
			{
				return root;
			}

			void AddXmlFragment(const char * contentStart, const char * elementStart) const
			{
				const auto size = static_cast<size_t>(elementStart-contentStart);
				if (size!=0)
				{
					current->AddChild({ contentStart, size });
				}
			}
		};

		inline void Generate(Evaluator & evaluator, const Node & node, std::ostream & out)
		{
			if (!node.enumeration.empty())
			{
				evaluator.ForEach(node.enumeration, [&](const ValueBase & value)
				{
					evaluator.Push(node.variable, value);
					SCOPE(pop, [&](){evaluator.Pop(node.variable);});

					for (const auto & child : node.children)
					{
						Generate(evaluator, child, out);
					}
				});
				return;
			}

			if (!node.value.empty())
			{
				std::regex r(propRegex);
				std::cregex_iterator it(node.value.data(), node.value.data() + node.value.size(), r);
				auto end = std::cregex_iterator{};
				if (it==end)
				{
					out << node.value;
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

			for (const auto & child : node.children)
			{
				Generate(evaluator, child, out);
			}
		}
	}

	inline void Generate(Evaluator & e, const std::string_view & xml, std::ostream & out)
	{
		Detail::Nodes nodes{xml};
		Detail::Generate(e, nodes.GetRoot(), out);
	}
}