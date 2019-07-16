#pragma once

#include "GLib/Xml/StateEngine.h"
#include "GLib/Xml/NameSpaceManager.h"
#include "GLib/Xml/Utils.h"

#include <stdexcept>
#include <sstream>

namespace GLib::Xml
{
	struct Attribute
	{
		std::string_view name;
		std::string_view value;
		std::string_view nameSpace; // order?
	};

	class AttributeIterator
	{
		Xml::StateEngine engine;
		const NameSpaceManager * const manager;
		const char * ptr;
		const char * end;
		const char * currentPtr;

		/////////// working data
		const char * attributeNameStart; // use pair?
		const char * attributeNameEnd;
		const char * attributeValueStart; // use pair?
		const char * attributeValueEnd;
		///////////

	public:
		AttributeIterator(const NameSpaceManager * manager, const char * begin, const char * end)
			: engine(State::ElementAttributeSpace)
			, manager(manager)
			, ptr(begin)
			, end(end)
			, currentPtr()
		{
			if (begin!=end)
			{
				Advance();
			}
		}

		AttributeIterator()
			: manager()
			, ptr()
			, end()
			, currentPtr()
		{
		}

		bool operator==(const AttributeIterator & other) const
		{
			return currentPtr == other.currentPtr;
		}

		bool operator!=(const AttributeIterator & it) const
		{
			return !(*this == it);
		}

		AttributeIterator & operator++()
		{
			Advance();
			return *this;
		}

		Attribute operator*() const
		{
			auto qName = Utils::ToStringView(attributeNameStart, attributeNameEnd);
			auto value = Utils::ToStringView(attributeValueStart, attributeValueEnd);

			if (!manager)
			{
				return { qName, value };
			}
			else
			{
				auto [name, nameSpace] = manager->Normalise(qName);
				return { name, value, nameSpace};
			}
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
			attributeNameStart = attributeNameEnd = attributeValueStart = attributeValueEnd = nullptr;

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
					// verify state == ?
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
						case Xml::State::ElementAttributeName:
						{
							attributeNameEnd = oldPtr;
							break;
						}

						case Xml::State::ElementAttributeValueQuote:
						case Xml::State::ElementAttributeValueSingleQuote:
						{
							attributeValueEnd = oldPtr;
							if (!manager || !manager->IsNameSpace(Utils::ToStringView(attributeNameStart, attributeNameEnd)))
							{
								return;
							}
							break;
						}

						default:;
					}

					switch (newState)
					{
						case Xml::State::ElementAttributeName:
						{
							attributeNameStart = oldPtr;
							break;
						}

						case Xml::State::ElementAttributeValueQuote:
						case Xml::State::ElementAttributeValueSingleQuote:
						{
							attributeValueStart = ptr;
							break;
						}

						default:;
					}
				}
			}
		}
	};
}
