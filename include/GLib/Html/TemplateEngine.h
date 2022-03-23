#pragma once

#include "Node.h"

#include <GLib/Eval/Evaluator.h>
#include <GLib/PairHash.h>
#include <GLib/Xml/Iterator.h>

#include <ostream>
#include <regex>

namespace GLib::Html
{
	class Generator
	{
		using AttributeMap = std::unordered_map<std::pair<std::string_view, std::string_view>, Xml::Attribute, Util::PairHash>;

		static constexpr auto nameSpace = std::string_view {"glib"};
		static constexpr auto block = std::string_view {"block"};
		static constexpr auto each = std::string_view {"each"};
		static constexpr auto if_ = std::string_view {"if"};
		static constexpr auto text = std::string_view {"text"};

		std::regex const propRegex {R"(\$\{([\w\.]+)\})"};
		std::regex const varRegex {R"(^(\w+)\s:\s\$\{([\w\.]+)\}$)"};

		Eval::Evaluator & evaluator;
		std::string_view textReplacement;

	public:
		Generator(Eval::Evaluator & evaluator)
			: evaluator(evaluator)
		{}

		void Generate(std::string_view xml, std::ostream & out)
		{
			Generate(Parse(xml), out);
		}

	private:
		static const char * EndOf(std::string_view value)
		{
			return value.data() + value.size();
		}

		Node Parse(const std::string_view xml)
		{
			Node root;
			Node * current = &root;

			Xml::Holder holder {xml};
			const auto & manager = holder.Manager();

			for (auto it = holder.begin(), end = holder.end(); it != end; ++it)
			{
				const Xml::Element & e = *it;

				if (e.NameSpace() == nameSpace && e.Name() == block)
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
								current->AddFragment(std::exchange(textReplacement, {}));
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
					for (const Xml::Attribute & attr : e.Attributes())
					{
						if (attr.Name == each)
						{
							eachValue = attr.Value;
						}
						else if (attr.Name == if_)
						{
							ifValue = attr.Value;
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

		Node * AddBlock(std::string_view eachValue, std::string_view ifValue, Node * node, size_t depth)
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
			bool modified = false;
			std::string_view textValue;
			std::string_view ifValue;
			std::string_view eachValue;

			std::string_view attr = e.Attributes().Value();
			Xml::Attributes attributes {attr, nullptr};
			bool pop {};

			// handle duplicate attr names?
			for (const Xml::Attribute & a : attributes)
			{
				if (!Xml::NameSpaceManager::IsDeclaration(a.Name))
				{
					auto [name, nameSpaceValue] = manager.Normalise(a.Name);
					atMap.emplace(std::make_pair(nameSpaceValue, name), a);
				}
			}

			for (auto [namespaceName, a] : atMap)
			{
				if (namespaceName.first == nameSpace)
				{
					if (namespaceName.second == if_)
					{
						ifValue = a.Value;
						modified = true;
					}
					else if (namespaceName.second == each)
					{
						eachValue = a.Value;
						modified = true;
					}
					else if (namespaceName.second == text)
					{
						if (e.Type() != Xml::ElementType::Open)
						{
							throw std::runtime_error("Misplaced Attribute");
						}
						textValue = a.Value;
						modified = true;
					}
					else
					{
						auto existingIt = atMap.find(std::make_pair("", namespaceName.second));
						if (existingIt == atMap.end())
						{
							throw std::runtime_error(std::string("Attribute not found : '") + std::string(namespaceName.second) + "'");
						}
						existingIt->second.Value = a.Value;
						modified = true;
					}
				}
			}

			if (!eachValue.empty())
			{
				std::match_results<std::string_view::const_iterator> m;
				std::regex_search(eachValue.begin(), eachValue.end(), m, varRegex);
				if (m.empty())
				{
					throw std::runtime_error("Error in each value : " + std::string(eachValue));
				}

				node = node->AddEnumeration(m[1], m[2], ifValue, e.Depth());
				pop = e.Type() == Xml::ElementType::Empty;
			}
			else if (!ifValue.empty())
			{
				node = node->AddEnumeration({}, {}, ifValue, e.Depth());
				pop = e.Type() == Xml::ElementType::Empty;
			}

			if (modified)
			{
				node->AddFragment(e.OuterXml().data(), attr.data() - 1);

				for (const Xml::Attribute & a : attributes)
				{
					std::string_view prefix = Xml::NameSpaceManager::CheckForDeclaration(a.Name);
					if (!prefix.empty())
					{
						std::string_view ns = manager.Get(prefix);
						if (ns == nameSpace)
						{
							continue;
						}

						node->AddFragment(" ");
						node->AddFragment(a.RawValue);
					}
					else
					{
						auto [name, nameSpaceValue] = manager.Normalise(a.Name);
						if (nameSpaceValue.empty())
						{
							node->AddFragment(" ");
							node->AddFragment(name.data(), a.Value.data());
							node->AddFragment(atMap[std::make_pair(nameSpaceValue, name)].Value);
							node->AddFragment(a.Value.data() - 1, a.Value.data());
						}
					}
				}
				node->AddFragment(EndOf(attr), EndOf(e.OuterXml()));
			}
			else
			{
				const char * p = e.OuterXml().data();
				for (const Xml::Attribute & a : attributes)
				{
					std::string_view prefix = Xml::NameSpaceManager::CheckForDeclaration(a.Name);
					if (!prefix.empty())
					{
						if (manager.Get(prefix) == nameSpace)
						{
							node->AddFragment(p, a.Name.data() - 1); // -1 minus space prefix
							p = EndOf(a.RawValue);
						}
					}
				}
				node->AddFragment(p, EndOf(e.OuterXml()));
			}

			if (pop)
			{
				node = node->Parent();
			}

			return textValue;
		}

		void Generate(const Node & node, std::ostream & out)
		{
			// todo eval during parse, store bool or property to evaluate
			std::string_view condition = node.Condition();
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

				std::string result = evaluator.Evaluate(m[1]);
				if (result == "false")
				{
					return;
				}

				if (result != "true")
				{
					throw std::runtime_error("Expected boolean value, got: " + result);
				}
			}

			if (!node.Enumeration().empty())
			{
				evaluator.ForEach(node.Enumeration(),
													[&](const Eval::ValueBase & value)
													{
														evaluator.Push(node.Variable(), value);

														for (const Node & child : node.Children())
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
				auto end = std::cregex_iterator {};
				if (it == end)
				{
					out << node.Value();
				}
				else
				{
					for (;;)
					{
						out << it->prefix();
						const auto & var = (*it)[1]; // +format;
						out << evaluator.Evaluate(var);
						auto suffix = it++->suffix(); // capture before ++
						if (it == end)
						{
							out << suffix;
							break;
						}
					}
				}
			}

			for (const Node & child : node.Children())
			{
				Generate(child, out);
			}
		}
	};

	inline void Generate(Eval::Evaluator & e, std::string_view xml, std::ostream & out)
	{
		Generator(e).Generate(xml, out);
	}
}