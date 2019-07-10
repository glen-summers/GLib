#pragma once

#include "GLib/Xml/StateEngine.h"

#include <iterator>
#include <string_view>
#include <sstream>
#include <unordered_map>
#include <stack>
#include <vector>

/*
Design assumptions:
xml input is a contiguous sequence of utf8 characters
string_view's are used to hold pieces of the xml input to avoid copying

attributes are currently collated into a vector
todo: 
just store a string_view and add an attribute iterator that doesnt copy\allocate
improve error msgs to include error detail line\column numbers
default namespace
standard entities
*/

namespace GLib::Xml
{
	namespace Detail
	{
		inline std::string_view ToStringView(const char * start, const char * end)
		{
			return {start, static_cast<size_t>(end - start) };
		}
	}

	struct Attribute;

	enum class ElementType : int
	{
		Open, Empty, Close
	};

	struct Element // class
	{
		std::string_view qName;
		std::string_view name;
		std::string_view nameSpace;
		std::string_view outerXml;
		ElementType type;
		std::vector<Attribute> attributes;
		size_t depth; // move/remove?

		Element(std::string_view qName, std::string_view name, std::string_view nameSpace, ElementType type, std::vector<Attribute> attributes)
			: qName(qName), name(name), nameSpace(nameSpace), type(type), attributes(attributes), depth()
		{}

		Element(std::string_view name, ElementType type, std::vector<Attribute> attributes)
			: qName(name), name(name), type(type), attributes(attributes), depth()
		{}

		Element() : type(), depth() {}
	};

	struct Attribute
	{
		std::string_view name;
		std::string_view value;
		std::string_view nameSpace;
	};

	class Iterator
	{
		inline static constexpr std::string_view NameSpaceAttribute = "xmlns";

		Xml::StateEngine engine;

		const char * ptr;
		const char * end;
		const char * currentPtr;

		/////////// element working data
		const char * start;
		const char * elementNameStart;
		const char * elementNameEnd;
		const char * attributeNameStart; // use pair?
		const char * attributeNameEnd;
		const char * attributeValueStart; // use pair?
		///////////

		Element element;

		std::unordered_map<std::string_view, std::string_view> nameSpaces;
		std::stack<std::pair<size_t, std::pair<std::string_view,std::string_view>>> nameSpaceStack;
		std::stack<std::string_view> elementStack;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = Element;
		using difference_type = void;
		using pointer = void;
		using reference = void;

		Iterator(const char * begin, const char * end)
			: ptr(begin)
			, end(end)
			, currentPtr()
			, start(begin)
			, elementNameStart()
			, elementNameEnd()
			, attributeNameStart()
			, attributeNameEnd()
			, attributeValueStart()
		{
			Advance();
		}

		Iterator() // end
			: ptr()
			, end()
			, currentPtr()
			, start()
			, elementNameStart()
			, elementNameEnd()
			, attributeNameStart()
			, attributeNameEnd()
			, attributeValueStart()
		{}

		// remove
		// hack to allow template engine to not specify nameSpace, but means input not valid xhtml
		void AddNameSpace(std::string_view name, std::string_view nameSpace)
		{
			nameSpaces.emplace(name, nameSpace);
		}

		bool operator==(const Iterator & other) const
		{
			return currentPtr == other.currentPtr;
		}

		bool operator!=(const Iterator & it) const
		{
			return !(*this == it);
		}

		Iterator & operator++()
		{
			Advance();
			return *this;
		}

		const Element & operator*() const
		{
			return element;
		}

		const Element * operator->() const
		{
			return &element;
		}

	private:
		[[noreturn]] void IllegalCharacter(char c) const
		{
			std::ostringstream s;
			s << "Illegal character: '" << *ptr << "' (0x" << std::hex << static_cast<unsigned>(c) << ')';
			throw std::runtime_error(s.str());
		}

		void Advance()
		{
			currentPtr = ptr;
			
			if (!currentPtr)
			{
				throw std::runtime_error("++end");
			}

			for (;;)
			{
				if (ptr == end)
				{
					currentPtr = nullptr;
					if (!engine.HasRootElement())
					{
						throw std::runtime_error("No root element");
					}

					if (engine.GetState() != Xml::State::Start || !elementStack.empty())
					{
						throw std::runtime_error("Xml not closed");
					}
					return;
				}

				const auto oldPtr = ptr;
				const auto oldState = engine.GetState();
				const auto newState = engine.Push(*ptr);
				if (newState == Xml::State::Error)
				{
					IllegalCharacter(*ptr);
				}
				++ptr;

				if (newState != oldState)
				{
					switch (oldState)
					{
						case Xml::State::Start:
						{
							element = {};
							break;
						}

						case Xml::State::ElementName:
						{
							elementNameEnd = oldPtr;
							element.type = ElementType::Open;
							break;
						}
						
						case Xml::State::ElementEndName:
						{
							elementNameEnd = oldPtr;
							element.type = ElementType::Close;
							break;
						}

						case Xml::State::ElementAttributeName:
						{
							attributeNameEnd = oldPtr;
							break;
						}

						case Xml::State::ElementAttributeValueQuote:
						case Xml::State::ElementAttributeValueSingleQuote:
						{
							const char * attributeValueEnd = oldPtr;
							element.attributes.emplace_back(Attribute
							{
								{attributeNameStart, static_cast<size_t>(attributeNameEnd - attributeNameStart)},
								{attributeValueStart+1, static_cast<size_t>(attributeValueEnd - attributeValueStart-1)},
								{}
							});
						}

						default:;
					}

					switch (newState)
					{
						case Xml::State::ElementName:
						{
							// now could be comment
							elementNameStart = oldPtr;
							break;
						}

						case Xml::State::Bang:
							elementNameStart = nullptr;
							break;

						case Xml::State::ElementEndName:
						{
							elementNameStart = oldPtr;
							break;
						}

						case Xml::State::EmptyElement:
						{
							element.type = ElementType::Empty;
							break;
						}

						case Xml::State::ElementAttributeName:
						{
							attributeNameStart = oldPtr;
							break;
						}

						case Xml::State::ElementAttributeValueQuote:
						case Xml::State::ElementAttributeValueSingleQuote:
						{
							attributeValueStart = oldPtr;
							break;
						}

						case Xml::State::Start:
						{
							// bug: white space at end is not in outerXml...
							// don't yield for !doctype atm
							// just set a member value for now?
							if (elementNameStart)
							{
								ProcessElement(ptr);
								start = ptr;
								return;
							}
						}

						default:;
					}
				}
			}
		}

		static void ValidateName(size_t colon, const std::string_view & value)
		{
			if (colon == 0 || value.find(':', colon+1) != std::string_view::npos)
			{
				throw std::runtime_error(std::string("Illegal name : '") + std::string(value) + '\'');
			}
		}

		void ProcessElement(const char * outerXmlEnd)
		{
			for (auto it = element.attributes.begin(); it!=element.attributes.end();)
			{
				if (it->name.compare(0, NameSpaceAttribute.size(), NameSpaceAttribute) == 0)
				{
					std::string_view nameSpace = it->value;
					std::string_view name = it->name.substr(NameSpaceAttribute.size()+1);

					auto nit = nameSpaces.find(name);
					if (nit!=nameSpaces.end())
					{
						nameSpaceStack.push({elementStack.size(), {name, nit->second}});
						nit->second = nameSpace;
					}
					else
					{
						nameSpaces.emplace(name, nameSpace);
					}
					it = element.attributes.erase(it);
				}
				else
				{
					++it;
				}
			}

			const std::string_view qName = Detail::ToStringView(elementNameStart, elementNameEnd);
			std::string_view name = qName, nameSpace;
			const size_t colon = qName.find(':');
			if (colon != std::string_view::npos)
			{
				ValidateName(colon, qName);
				nameSpace = qName.substr(0, colon);
				const auto it = nameSpaces.find(nameSpace);
				if (it == nameSpaces.end())
				{
					throw std::runtime_error(std::string("NameSpace ") + std::string(nameSpace) + " not found");
				}
				name = qName.substr(colon+1);
				nameSpace = it->second;
			}

			element.qName = qName;
			element.name = name;
			element.nameSpace = nameSpace;
			element.outerXml = Detail::ToStringView(start, outerXmlEnd);

			for (auto & at : element.attributes)
			{
				const size_t atColon = at.name.find(':');
				if (atColon != std::string_view::npos)
				{
					ValidateName(atColon, at.name);
					auto atNameSpace = at.name.substr(0, atColon);
					auto it = nameSpaces.find(atNameSpace);
					if (it == nameSpaces.end())
					{
						throw std::runtime_error(std::string("NameSpace ") + std::string(atNameSpace) + " not found");
					}
					at.name = at.name.substr(atColon+1);
					at.nameSpace = it->second;
				}
			}

			switch (element.type)
			{
				case ElementType::Open:
				{
					elementStack.push(element.qName);
					element.depth = elementStack.size();
					break;
				}

				case ElementType::Empty:
				{
					element.depth = elementStack.size() + 1;
					break;
				}

				case ElementType::Close:
				{
					element.depth = elementStack.size();
					if (element.depth == 0)
					{
						throw std::runtime_error("Extra content at document end");
					}
					const auto & top = elementStack.top();
					if (element.qName != top)
					{
						std::ostringstream s;
						s << "Element mismatch: "  << element.qName << " != " << top;
						throw std::runtime_error(s.str());
					}
					elementStack.pop();
					break;
				}

				default:
				{
					throw std::logic_error("Unexpected enumeration value");
				}
			}

			while (!nameSpaceStack.empty() && elementStack.size() == nameSpaceStack.top().first)
			{
				auto nit = nameSpaces.find(nameSpaceStack.top().second.first);
				if (nit==nameSpaces.end())
				{
					throw std::logic_error("!");
				}
				nit->second = nameSpaceStack.top().second.second;
				nameSpaceStack.pop();
			}
		}
	};

	class Holder
	{
		std::string_view const value;

	public:
		Holder(std::string_view value)
			: value(value)
		{}

		Iterator begin() const
		{
			return Iterator{value.data(), value.size() + value.data()};
		}

		Iterator end()
		{
			(void)this;
			return Iterator{};
		}
	};
}