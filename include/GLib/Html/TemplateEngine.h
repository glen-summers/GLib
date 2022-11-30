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

		static constexpr auto mainNameSpace = std::string_view {"glib"};
		static constexpr auto block = std::string_view {"block"};
		static constexpr auto each = std::string_view {"each"};
		static constexpr auto if_ = std::string_view {"if"};
		static constexpr auto text = std::string_view {"text"};

		std::regex const propRegex {R"(\$\{([\w\.]+)\})"};
		std::regex const varRegex {R"(^(\w+)\s:\s\$\{([\w\.]+)\}$)"};

		Eval::Evaluator & evaluator;
		std::string_view textReplacement;

	public:
		explicit Generator(Eval::Evaluator & evaluator)
			: evaluator(evaluator)
		{}

		void Generate(std::string_view const xml, std::ostream & out)
		{
			Generate(Parse(xml), out);
		}

	private:
		[[nodiscard]] static char const * EndOf(std::string_view const value)
		{
			return value.data() + value.size();
		}

		[[nodiscard]] Node Parse(std::string_view const xml)
		{
			Node root;
			Node * current = &root;

			Xml::Holder holder {xml};
			auto const & manager = holder.Manager();

			for (auto it = holder.begin(), end = holder.end(); it != end; ++it)
			{
				Xml::Element const & e = *it;

				if (e.NameSpace() == mainNameSpace && e.Name() == block)
				{
					current = ProcessBlock(e, current);
				}
				else
				{
					current = ProcessNonBlock(current, manager, e);
				}
			}

			return root;
		}

		[[nodiscard]] static auto ProcessIfEach(Xml::Element const & e)
		{
			std::string_view eachValue;
			std::string_view ifValue;
			for (auto const && [name, value, nameSpace, rawValue] : e.GetAttributes())
			{
				if (name == each)
				{
					eachValue = value;
				}
				else if (name == if_)
				{
					ifValue = value;
				}
			}

			if (eachValue.empty() && ifValue.empty())
			{
				throw std::runtime_error {"No action attribute"};
			}

			return make_tuple(eachValue, ifValue);
		}

		[[nodiscard]] Node * ProcessBlock(Xml::Element const & e, Node * node) const
		{
			switch (e.Type())
			{
				case Xml::ElementType::Open:
				{
					auto [eachValue, ifValue] = ProcessIfEach(e);
					return AddBlock(eachValue, ifValue, node, e.Depth());
				}

				case Xml::ElementType::Empty:
				{
					throw std::runtime_error {"No block content"};
				}

				case Xml::ElementType::Close:
				{
					if (node == nullptr)
					{
						throw std::logic_error {"No parent node"};
					}
					return node->Parent();
				}

				case Xml::ElementType::Text:
				case Xml::ElementType::Comment:
				{
					break;
				}
			}
			throw std::logic_error {"Unexpected enumeration value"};
		}

		[[nodiscard]] Node * ProcessNonBlock(Node * current, Xml::NameSpaceManager const & manager, Xml::Element const & e)
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
			return current;
		}

		[[nodiscard]] Node * AddBlock(std::string_view eachValue, std::string_view const ifValue, Node * const node, size_t const depth) const
		{
			if (!eachValue.empty())
			{
				std::match_results<std::string_view::const_iterator> m;
				std::regex_search(eachValue.begin(), eachValue.end(), m, varRegex);
				if (m.empty())
				{
					throw std::runtime_error {"Error in each value : " + std::string {eachValue}};
				}

				node->AddEnumeration(m[1], m[2], ifValue, depth);
				return node->Back();
			}

			node->AddConditional(ifValue);
			return node->Back();
		}

		[[nodiscard]] static Node * ProcessAttributes(Node * node, Xml::NameSpaceManager const & manager, AttributeMap const & atMap,
																									Xml::Attributes const & attributes)
		{
			for (auto const && [name2, value2, nameSpace2, rawValue2] : attributes)
			{
				if (std::string_view prefix = Xml::NameSpaceManager::CheckForDeclaration(name2); !prefix.empty())
				{
					if (std::string_view const ns = manager.Get(prefix); ns == mainNameSpace)
					{
						continue;
					}

					node->AddFragment(" ");
					node->AddFragment(rawValue2);
				}
				else
				{
					auto && [name, nameSpace] = manager.Normalise(name2);
					if (nameSpace.empty())
					{
						node->AddFragment(" ");
						node->AddFragment(name.data(), value2.data());

						auto const it = atMap.find(std::make_pair(nameSpace, name));
						if (it == atMap.cend())
						{
							throw std::runtime_error {"Namespace not found: " + std::string {nameSpace}};
						}
						node->AddFragment(it->second.Value);
						node->AddFragment(value2.data() - 1, value2.data());
					}
				}
			}
			return node;
		}

		[[nodiscard]] static Node * ProcessDeclarations(Xml::Element const & e, Node * node, Xml::NameSpaceManager const & manager,
																										Xml::Attributes const & attributes)
		{
			char const * p = e.OuterXml().data();
			for (auto const && [name, value, nameSpace, rawValue] : attributes)
			{
				if (std::string_view prefix = Xml::NameSpaceManager::CheckForDeclaration(name); !prefix.empty() && manager.Get(prefix) == mainNameSpace)
				{
					node->AddFragment(p, name.data() - 1); // -1 minus space prefix
					p = EndOf(rawValue);
				}
			}
			node->AddFragment(p, EndOf(e.OuterXml()));
			return node;
		}

		[[nodiscard]] std::string_view ProcessElement(Xml::Element const & e, Node *& node, Xml::NameSpaceManager const & manager) const
		{
			AttributeMap atMap;
			bool modified = false;
			std::string_view textValue;
			std::string_view ifValue;
			std::string_view eachValue;

			std::string_view attributesValue = e.GetAttributes().Value();
			Xml::Attributes attributes {attributesValue, nullptr};
			bool pop {};

			// handle duplicate attr names?
			for (Xml::Attribute const && a : attributes)
			{
				if (!Xml::NameSpaceManager::IsDeclaration(a.Name))
				{
					auto [name, nameSpace] = manager.Normalise(a.Name);
					atMap.emplace(std::make_pair(nameSpace, name), a);
				}
			}

			for (auto && [nameSpaceName, a] : atMap)
			{
				if (nameSpaceName.first != mainNameSpace)
				{
					continue;
				}

				if (nameSpaceName.second == if_)
				{
					ifValue = a.Value;
					modified = true;
					continue;
				}

				if (nameSpaceName.second == each)
				{
					eachValue = a.Value;
					modified = true;
					continue;
				}

				if (nameSpaceName.second == text && e.Type() != Xml::ElementType::Open)
				{
					throw std::runtime_error {"Misplaced Attribute"};
				}

				if (nameSpaceName.second == text && e.Type() == Xml::ElementType::Open)
				{
					textValue = a.Value;
					modified = true;
					continue;
				}

				if (auto existingIt = atMap.find(std::make_pair("", nameSpaceName.second)); existingIt != atMap.end())
				{
					existingIt->second.Value = a.Value;
					modified = true;
					continue;
				}

				throw std::runtime_error {"Attribute not found : '" + std::string {nameSpaceName.second} + "'"};
			}

			if (!eachValue.empty())
			{
				std::match_results<std::string_view::const_iterator> m;
				std::regex_search(eachValue.begin(), eachValue.end(), m, varRegex);
				if (m.empty())
				{
					throw std::runtime_error("Error in each value : " + std::string(eachValue));
				}

				node->AddEnumeration(m[1], m[2], ifValue, e.Depth());
				node = node->Back();
				pop = e.Type() == Xml::ElementType::Empty;
			}
			else if (!ifValue.empty())
			{
				node->AddEnumeration({}, {}, ifValue, e.Depth());
				node = node->Back();
				pop = e.Type() == Xml::ElementType::Empty;
			}

			if (modified)
			{
				node->AddFragment(e.OuterXml().data(), attributesValue.data() - 1);
				node = ProcessAttributes(node, manager, atMap, attributes);
				node->AddFragment(EndOf(attributesValue), EndOf(e.OuterXml()));
			}
			else
			{
				node = ProcessDeclarations(e, node, manager, attributes);
			}

			if (pop)
			{
				node = node->Parent();
			}

			return textValue;
		}

		void Generate(Node const & node, std::ostream & out)
		{
			// todo eval during parse, store bool or property to evaluate
			std::string_view condition = node.Condition();
			if (condition == "false")
			{
				return;
			}

			if (!condition.empty() && condition != "true")
			{
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
				auto subGenerate = [&](Eval::ValueBase const & value)
				{
					evaluator.Push(node.Variable(), value);

					for (Node const & child : node.Children())
					{
						Generate(child, out);
					}

					evaluator.Pop(node.Variable());
				};

				evaluator.ForEach(node.Enumeration(), subGenerate);
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
						auto const & var = (*it)[1]; // +format;
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

			for (Node const & child : node.Children())
			{
				Generate(child, out);
			}
		}
	};

	inline void Generate(Eval::Evaluator & e, std::string_view const xml, std::ostream & out)
	{
		Generator(e).Generate(xml, out);
	}
}