#pragma once

#include "GLib/Xml/StateEngine.h"
#include "GLib/Xml/NameSpacemanager.h"
#include "GLib/Xml/Element.h"

#include <iterator>
#include <sstream>

/*
Design:
xml input is a contiguous sequence of utf8 characters
string_view's are used to hold pieces of the xml input to avoid copying
seperatet attribute iterator exposed, enumerated first for namespaces then for values

todo:
improve error msgs to include error detail line\column numbers
default namespace
standard entities
*/

namespace GLib::Xml
{
	class Iterator
	{
		friend class AttributeIterator;

		Xml::StateEngine engine;
		NameSpaceManager manager;

		const char * ptr;
		const char * end;
		const char * currentPtr;

		/////////// element working data, could just use element storage
		const char * start;
		const char * elementNameStart;
		const char * elementNameEnd;
		const char * attributesStart; // use pair?
		const char * attributesEnd; // could remove, use state change

		const char * attributeNameStart;
		const char * attributeNameEnd;
		const char * attributeValueStart;
		///////////

		Element element;
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
			, attributesStart()
			, attributesEnd()
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
			, attributesStart()
			, attributesEnd()
			, attributeNameStart()
			, attributeNameEnd()
			, attributeValueStart()
		{}

		// remove
		// hack to allow template engine to not specify nameSpace, but means input not valid xhtml
		void AddNameSpace(std::string_view name, std::string_view nameSpace)
		{
			manager.Add(name, nameSpace);
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
				// should not use ptr below here as could now be end!
				// would only happen with truncated xml but still fix

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
							if (newState == State::ElementAttributeSpace)
							{
								attributesStart = ptr;
								attributesEnd = nullptr;
							}
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
							manager.Check(Utils::ToStringView(attributeNameStart, attributeNameEnd),
								Utils::ToStringView(attributeValueStart+1, oldPtr), elementStack.size());

							// better test? currently writes ones per attr
							attributesEnd = ptr;
							break;
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
							break;
						}

						default:;
					}
				}
			}
		}

		void ProcessNameSpaces(const std::string_view & attributes) const
		{
			for (auto at : Attributes{attributes, &manager})
			{
				(void)at;
			}
		}

		void ProcessElement(const char * outerXmlEnd)
		{
			std::string_view attributes;
			if (attributesStart && attributesEnd && element.type != ElementType::Close)
			{
				attributes = Utils::ToStringView(attributesStart, attributesEnd);
			}
			ProcessNameSpaces(attributes);

			auto qName = Utils::ToStringView(elementNameStart, elementNameEnd);
			auto [name, nameSpace] = manager.Normalise(qName);

			element.qName = qName;
			element.name = name;
			element.nameSpace = nameSpace;
			element.outerXml = Utils::ToStringView(start, outerXmlEnd);
			element.attributes = attributes;

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

			manager.Pop(elementStack.size());
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