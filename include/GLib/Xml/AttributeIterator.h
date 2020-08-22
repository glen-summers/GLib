#pragma once

#include <GLib/Xml/NameSpaceManager.h>
#include <GLib/Xml/StateEngine.h>
#include <GLib/Xml/Utils.h>

#include <sstream>
#include <stdexcept>

namespace GLib::Xml
{
	struct Attribute
	{
		std::string_view name;
		std::string_view value;
		std::string_view nameSpace;
		std::string_view rawValue;
	};

	class AttributeIterator
	{
		Xml::StateEngine engine;
		const NameSpaceManager * manager {};
		const char * ptr {};
		const char * end {};
		const char * currentPtr {};

		/////////// working data
		Utils::PtrPair attributeName;
		Utils::PtrPair attributeValue;
		///////////

	public:
		AttributeIterator(const NameSpaceManager * manager, const char * begin, const char * end)
			: engine(State::AttributeSpace)
			, manager(manager)
			, ptr(begin)
			, end(end)
		{
			if (begin!=end)
			{
				Advance();
			}
		}

		AttributeIterator() = default;

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
			auto qName = Utils::ToStringView(attributeName);
			auto value = Utils::ToStringView(attributeValue);
			auto rawValue = Utils::ToStringView(attributeName.first, attributeValue.second+1);

			if (manager != nullptr)
			{
				auto [name, nameSpace] = manager->Normalise(qName);
				return { name, value, nameSpace, rawValue };
			}
			return { qName, value, {}, rawValue };
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
			attributeName = attributeValue = {};
			currentPtr = ptr;

			if (currentPtr == nullptr)
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

				const auto * const oldPtr = ptr;
				const auto oldState = engine.GetState();
				const auto newState = engine.Push(*ptr);
				if (newState == Xml::State::Error)
				{
					IllegalCharacter(*ptr);
				}
				++ptr; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) todo use std::span

				if (newState != oldState)
				{
					if (newState == Xml::State::AttributeEntity || oldState == Xml::State::AttributeEntity)
					{
						continue; // currenly ignoring entities, receiver needs to decode
					}

					switch (oldState)
					{
						case Xml::State::AttributeName:
						{
							attributeName.second = oldPtr;
							break;
						}

						case Xml::State::AttributeValue:
						{
							attributeValue.second = oldPtr;
							if (manager == nullptr || !Xml::NameSpaceManager::IsDeclaration(Utils::ToStringView(attributeName)))
							{
								return;
							}
							break;
						}

						default:
						{
							break;
						}
					}

					switch (newState)
					{
						case Xml::State::AttributeName:
						{
							attributeName.first = oldPtr;
							break;
						}

						case Xml::State::AttributeValue:
						{
							attributeValue.first = ptr;
							break;
						}

						default:
						{
							break;
						}
					}
				}
			}
		}
	};
}
