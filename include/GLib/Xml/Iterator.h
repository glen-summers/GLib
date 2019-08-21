#pragma once

#include "GLib/Xml/Element.h"
#include "GLib/Xml/NameSpaceManager.h"
#include "GLib/Xml/StateEngine.h"

#include <iterator>
#include <sstream>

/*
Design:
xml input is a contiguous sequence of utf8 characters
string_view's are used to hold pieces of the xml input to avoid copying
separate attribute iterator exposed, enumerated first for namespaces then for values

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

		StateEngine engine;
		NameSpaceManager manager;

		const char * ptr {};
		const char * end {};
		const char * lastPtr {};

		/////////// element working data, could just use element storage
		Utils::PtrPair elementName;
		Utils::PtrPair attributes;
		Utils::PtrPair attributeName;
		const char * attributeValueStart {};
		bool contentClosed {};
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
			, lastPtr(begin)
		{
			Advance();
		}

		Iterator() = default; // end

		const NameSpaceManager & Manager() const
		{
			return manager;
		}

		bool operator==(const Iterator & other) const
		{
			return lastPtr == other.lastPtr;
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
			if (lastPtr == nullptr)
			{
				throw std::runtime_error("++end");
			}

			for (;;)
			{
				if (ptr == end)
				{
					lastPtr = nullptr;
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
				++ptr; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) todo use std::span
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
							elementName.second = oldPtr;
							element.type = ElementType::Open;
							if (newState == State::ElementAttributeSpace)
							{
								attributes = {ptr, nullptr};
							}
							break;
						}
						
						case Xml::State::ElementEndName:
						{
							elementName.second = oldPtr;
							element.type = ElementType::Close;
							break;
						}

						case Xml::State::ElementAttributeName:
						{
							attributeName.second = oldPtr;
							break;
						}

						case Xml::State::ElementAttributeValueQuote:
						case Xml::State::ElementAttributeValueSingleQuote:
						{
							manager.Push(Utils::ToStringView(attributeName), Utils::ToStringView(attributeValueStart, oldPtr).substr(1), elementStack.size());

							// better test? currently writes ones per attr
							attributes.second = ptr;
							break;
						}

						default:;
					}

					switch (newState)
					{
						case Xml::State::ElementName:
						case Xml::State::ElementEndName:
						{
							elementName.first = oldPtr;
							break;
						}

						case Xml::State::Bang:
						{
							elementName.first = nullptr;
							break;
						}

						case Xml::State::EmptyElement:
						{
							element.type = ElementType::Empty;
							break;
						}

						case Xml::State::ElementAttributeName:
						{
							attributeName.first = oldPtr;
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
							if (elementName.first != nullptr)
							{
								ProcessElement(ptr);
								lastPtr = ptr;
								return;
							}
							break;
						}

						default:;
					}
				}
			}
		}

		void ProcessElement(const char * outerXmlEnd)
		{
			if (contentClosed)
			{
				throw std::runtime_error("Extra content at document end");
			}

			auto qName = Utils::ToStringView(elementName);
			auto [name, nameSpace] = manager.Normalise(qName);

			element.qName = qName;
			element.name = name;
			element.nameSpace = nameSpace;
			element.outerXml = Utils::ToStringView(lastPtr, outerXmlEnd);

			if (attributes.first != nullptr && attributes.second != nullptr && element.type != ElementType::Close)
			{
				element.attributes = {Utils::ToStringView(attributes), &manager};
			}
			else
			{
				element.attributes = {};
			}
			attributes = {};

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
					manager.Pop(elementStack.size());
					if (element.depth == 1)
					{
						contentClosed = true;
					}
					break;
				}

				case ElementType::Close:
				{
					element.depth = elementStack.size();
					const auto & top = elementStack.top();
					if (element.qName != top)
					{
						std::ostringstream s;
						s << "Element mismatch: "  << element.qName << " != " << top;
						throw std::runtime_error(s.str());
					}
					if (element.depth == 1)
					{
						contentClosed = true;
					}
					elementStack.pop();
					manager.Pop(elementStack.size());
					break;
				}

				default:
				{
					throw std::logic_error("Unexpected enumeration value");
				}
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

		Iterator end() const
		{
			(void)this;
			return Iterator{};
		}
	};
}