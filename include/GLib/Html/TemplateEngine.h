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
		static constexpr auto If = std::string_view {"if"};
		static constexpr auto Text = std::string_view {"text"};

		std::regex const propRegex { R"(\$\{([\w\.]+)\})" };
		std::regex const varRegex { R"(^(\w+)\s:\s\$\{([\w\.]+)\}$)" };

		Eval::Evaluator & evaluator;
		std::string_view textReplacement;

	public:
		Generator(Eval::Evaluator & evaluator)
			: evaluator(evaluator)
		{}

		void Generate(const std::string_view & xml, std::ostream & out)
		{
			Generate(Parse(xml), out);
		}

	private:
		static const char * EndOf(const std::string_view & value)
		{
			return value.data() + value.size(); // &*value.rbegin()+1;
		}

		Node Parse(const std::string_view xml)
		{
			Node root;
			Node * current = &root;

			Xml::Holder holder{xml};
			const auto & manager = holder.Manager();

			for (auto it = holder.begin(), end = holder.end(); it != end; ++it)
			{
				const Xml::Element & e = *it;

				if (e.NameSpace() == NameSpace && e.Name() == Block)
				{
					current = ProcessBlock(e, current);
				}
				else
				{
					switch (e.Type())
					{
						case Xml::ElementType::Open:
						case Xml::ElementType::Empty:
						{
							textReplacement = ProcessElement(e, current, manager);
							break;
						}

						case Xml::ElementType::Close:
						{
							current->AddFragment(e.OuterXml());
							if (current->Depth() == e.Depth())
							{
								current = current->Parent();
							}
							break;
						}

						case Xml::ElementType::Text:
						{
							if (textReplacement.empty())
							{
								// loses whitespace
								current->AddFragment(e.Text());
							}
							else
							{
								current->AddFragment(std::exchange(textReplacement,{}));
							}

							break;
						}

						case Xml::ElementType::Comment:
						{
							current->AddFragment(e.Text());
							break;
						}
					}
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
					std::string_view eachValue;
					std::string_view ifValue;
					for (auto attr : e.Attributes())
					{
						if (attr.name == Each)
						{
							eachValue = attr.value;
						}
						else if (attr.name == If)
						{
							ifValue = attr.value;
						}
					}

					if (eachValue.empty() && ifValue.empty())
					{
						throw std::runtime_error("No action attribute");
					}

					return AddBlock(eachValue, ifValue, node, e.Depth());
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

		Node* AddBlock(const std::string_view & eachValue, const std::string_view & ifValue, Node * node, size_t depth)
		{
			if (!eachValue.empty())
			{
				std::match_results<std::string_view::const_iterator> m;
				std::regex_search(eachValue.begin(), eachValue.end(), m, varRegex);
				if (m.empty())
				{
					throw std::runtime_error("Error in each value : " + std::string(eachValue));
				}

				return node->AddEnumeration(m[1], m[2], ifValue, depth);
			}

			return node->AddConditional(ifValue);
		}

		std::string_view ProcessElement(const Xml::Element & e, Node *& node, const Xml::NameSpaceManager & manager)
		{
			AttributeMap atMap;
			bool replaced = false;
			std::string_view text;
			std::string_view each;
			auto attr = e.Attributes().Value();
			Xml::Attributes attributes { e.Attributes().Value(), nullptr };
			bool pop{};

			for (auto a : attributes)
			{
				if (!Xml::NameSpaceManager::IsDeclaration(a.name))
				{
					auto [name, nameSpace] = manager.Normalise(a.name);
					atMap.emplace(std::make_pair(nameSpace, name), a);
				}
			}

			for (auto [namespaceName, a] : atMap)
			{
				if (namespaceName.first == NameSpace)
				{
					// also if attr?
					if (namespaceName.second == Each)
					{
						each = a.value;
					}
					else if (namespaceName.second == Text)
					{
						if (e.Type() != Xml::ElementType::Open)
						{
							throw std::runtime_error("Misplaced Attribute");
						}
						text = a.value;
					}
					else
					{
						auto existingIt = atMap.find(std::make_pair("", namespaceName.second));
						if (existingIt == atMap.end())
						{
							throw std::runtime_error(std::string("Attribute not found : '") + std::string(namespaceName.second) + "'");
						}
						existingIt->second.value = a.value;
						replaced = true;
					}
				}
			}

			if (!each.empty())
			{
				std::match_results<std::string_view::const_iterator> m;
				std::regex_search(each.begin(), each.end(), m, varRegex);
				if (m.empty())
				{
					throw std::runtime_error("Error in each value : " + std::string(each));
				}

				node = node->AddEnumeration(m[1], m[2], {}, e.Depth());
				pop = e.Type() == Xml::ElementType::Empty;
			}

			if (replaced || !text.empty() || !each.empty())
			{
				node->AddFragment(e.OuterXml().data(), attr.data()-1);

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

						node->AddFragment(" ");
						node->AddFragment(a.rawValue);
					}
					else
					{
						auto [name, nameSpace] = manager.Normalise(a.name);
						if (nameSpace.empty())
						{
							node->AddFragment(" ");
							node->AddFragment(name.data(), a.value.data());
							node->AddFragment(atMap[std::make_pair(nameSpace, name)].value);
							node->AddFragment(a.value.data() - 1, a.value.data());
						}
					}
				}
				node->AddFragment(EndOf(attr), EndOf(e.OuterXml()));
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
							node->AddFragment(p, a.name.data() - 1); // -1 minus space prefix
							p = EndOf(a.value);
							++p; // +1 trailing quote
						}
					}
				}
				node->AddFragment(p, EndOf(e.OuterXml()));
			}

			if (pop)
			{
				node = node->Parent();
			}

			return text;
		}

		void Generate(const Node & node, std::ostream & out)
		{
			// todo eval during parse, store bool or property to evaluate
			const std::string_view & condition = node.Condition();
			if (!condition.empty() && condition != "true")
			{
				if (condition == "false")
				{
					return;
				}

				std::match_results<std::string_view::const_iterator> m;
				std::regex_search(condition.begin(), condition.end(), m, propRegex);
				if (m.empty())
				{
					throw std::runtime_error("Error in if value : " + std::string(condition));
				}

				auto result = evaluator.Evaluate(m[1]);
				if (result == "false")
				{
					return;
				}

				if (result !="true")
				{
					throw std::runtime_error("Expected boolean value, got: " + result);
				}
			}

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
				std::cregex_iterator it(node.Value().data(), EndOf(node.Value()), r);
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

	inline void Generate(Eval::Evaluator & e, const std::string_view & xml, std::ostream & out)
	{
		Generator(e).Generate(xml, out);
	}
}