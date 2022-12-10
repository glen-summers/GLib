#pragma once

#include <GLib/Xml/Element.h>
#include <GLib/Xml/NameSpaceManager.h>
#include <GLib/Xml/StateEngine.h>
#include <GLib/Xml/Utils.h>

#include <iterator>
#include <sstream>

/*
Design:
xml input is a contiguous sequence of utf8 characters
string_view's are used to hold pieces of the xml input to avoid copying
separate attribute iterator exposed, enumerated first for namespaces then for values

todo:
improve error message to include error detail line\column numbers
default namespace
standard entities
*/

namespace GLib::Xml
{
	class Iterator
	{
		friend class AttributeIterator;
		static constexpr char minPrintable = 0x20;

		StateEngine engine;

		std::string_view::const_iterator ptr {};
		std::string_view::const_iterator const end {};
		std::optional<std::string_view::const_iterator> lastPtr;
		NameSpaceManager * manager = {};

		/////////// element working data, could just use element storage
		ElementType elementType {};
		Utils::PtrPair elementName;
		Utils::PtrPair attributes;
		Utils::PtrPair attributeName;

		std::string_view::const_iterator attributeValueStart;

		bool contentClosed {};
		///////////

		Element element;
		std::stack<std::string_view> elementStack;
		unsigned int line {};
		unsigned int pos {};

	public:
		// ReSharper disable All
		using iterator_category = std::forward_iterator_tag;
		using value_type = Element;
		using difference_type = void;
		using pointer = void;
		using reference = void;
		// ReSharper restore All

		Iterator(std::string_view::const_iterator begin, std::string_view::const_iterator const end, NameSpaceManager * manager)
			: ptr(begin)
			, end(end)
			, lastPtr(begin)
			, manager(manager)
		{
			Advance();
		}

		Iterator() = default;

		bool operator==(Iterator const & other) const
		{
			return lastPtr == other.lastPtr;
		}

		bool operator!=(Iterator const & iter) const
		{
			return !(*this == iter);
		}

		Iterator & operator++()
		{
			Advance();
			return *this;
		}

		Element const & operator*() const
		{
			return element;
		}

		Element const * operator->() const
		{
			return &element;
		}

	private:
		[[noreturn]] static void IllegalCharacter(char const chr, unsigned int const lineNumber, unsigned int const characterOffset)
		{
			std::ostringstream stm;
			stm << "Illegal character: ";
			if (chr >= minPrintable)
			{
				stm << '\'' << chr << "' ";
			}

			stm << "(0x" << std::hex << static_cast<unsigned>(chr) << std::dec << ")"
					<< " at line: " << lineNumber << ", offset: " << characterOffset;

			throw std::runtime_error(stm.str());
		}

		bool ProcessOldState(std::string_view::const_iterator const oldPtr, State const oldState, State const newState)
		{
			if (oldState == State::Text && newState == State::TextEntity)
			{
				return false;
			}

			if (oldState == State::Text)
			{
				element = {ElementType::Text, Utils::ToStringView({*lastPtr, oldPtr})};
				lastPtr = oldPtr;
				return true;
			}

			if (oldState == State::CommentEnd)
			{
				element = {ElementType::Comment, Utils::ToStringView({*lastPtr, ptr})};
				lastPtr = ptr;
				return true;
			}

			if (oldState == State::ElementName)
			{
				elementName.second = oldPtr;
				elementType = ElementType::Open;
				if (newState == State::AttributeSpace)
				{
					attributes = {ptr, ptr};
					return false;
				}
			}

			if (oldState == State::ElementEndName)
			{
				elementName.second = oldPtr;
				elementType = ElementType::Close;
				return false;
			}

			if (oldState == State::AttributeName)
			{
				attributeName.second = oldPtr;
				return false;
			}

			return false;
		}

		bool ProcessNewState(std::string_view::const_iterator const oldPtr, State const newState)
		{
			if (newState == State::AttributeEnd)
			{
				manager->Push(Utils::ToStringView(attributeName), Utils::ToStringView({attributeValueStart, oldPtr}).substr(1), elementStack.size());
				attributes.second = ptr;
				return false;
			}

			if (newState == State::ElementName || newState == State::ElementEndName || newState == State::Bang)
			{
				elementName = {oldPtr, oldPtr};
				return false;
			}

			if (newState == State::EmptyElement)
			{
				elementType = ElementType::Empty;
				return false;
			}

			if (newState == State::AttributeName)
			{
				attributeName.first = oldPtr;
				return false;
			}

			if (newState == State::AttributeValue)
			{
				attributeValueStart = oldPtr;
				return false;
			}

			if (newState == State::Start)
			{
				// bug: white space at end is not in outerXml...
				// don't yield for !doctype atm
				// just set a member value for now?

				if (elementName.first == elementName.second) // skip doctype
				{
					return false;
				}

				if (elementName.first != end && elementName.first != elementName.second)
				{
					ProcessElement(ptr);
					lastPtr = ptr;
					return true;
				}
			}

			return false;
		}

		void Advance()
		{
			if (!lastPtr.has_value())
			{
				throw std::runtime_error("++end");
			}

			for (;;)
			{
				if (ptr == end)
				{
					lastPtr.reset();
					if (!engine.HasRootElement())
					{
						throw std::runtime_error("No root element");
					}

					if (engine.GetState() != State::Start || !elementStack.empty())
					{
						throw std::runtime_error("Xml not closed");
					}
					return;
				}

				auto const oldPtr = ptr;
				auto const oldState = engine.GetState();
				char const character = *ptr;
				auto const newState = engine.Push(character);

				if (newState == State::Error)
				{
					IllegalCharacter(character, line, pos);
				}
				++pos;

				if (character == '\n')
				{
					++line;
					pos = 0;
				}

				++ptr;

				if (newState != oldState)
				{
					if (ProcessOldState(oldPtr, oldState, newState))
					{
						return;
					}

					if (ProcessNewState(oldPtr, newState))
					{
						return;
					}
				}
			}
		}

		void ProcessElement(std::string_view::const_iterator outerXmlEnd)
		{
			if (contentClosed)
			{
				throw std::runtime_error("Extra content at document end");
			}

			auto qName = Utils::ToStringView(elementName);
			auto [name, nameSpace] = manager->Normalise(qName);

			element = {qName, name, nameSpace, elementType};
			element.outerXml = Utils::ToStringView({*lastPtr, outerXmlEnd});

			if (attributes.first != attributes.second && attributes.first != end && attributes.second != end && element.type != ElementType::Close)
			{
				element.attributes = {Utils::ToStringView(attributes), manager};
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
					manager->Pop(elementStack.size());
					if (element.depth == 1)
					{
						contentClosed = true;
					}
					break;
				}

				case ElementType::Close:
				{
					element.depth = elementStack.size();
					auto const & top = elementStack.top();
					if (element.qName != top)
					{
						std::ostringstream stm;
						stm << "Element mismatch: " << element.qName << " != " << top << ", at line: " << line << ", offset: " << pos;
						throw std::runtime_error(stm.str());
					}
					if (element.depth == 1)
					{
						contentClosed = true;
					}
					elementStack.pop();
					manager->Pop(elementStack.size());
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
		NameSpaceManager manager;

	public:
		explicit Holder(std::string_view const value)
			: value(value)
		{}

		Iterator begin()
		{
			return {value.begin(), value.end(), &manager};
		}

		[[nodiscard]] Iterator end() const
		{
			static_cast<void>(this);
			return Iterator {};
		}

		NameSpaceManager const & Manager()
		{
			return manager;
		}
	};
}
