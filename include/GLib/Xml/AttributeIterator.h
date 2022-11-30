#pragma once

#include <GLib/Xml/NameSpaceManager.h>
#include <GLib/Xml/StateEngine.h>
#include <GLib/Xml/Utils.h>

#include <optional>
#include <sstream>
#include <stdexcept>

namespace GLib::Xml
{
	struct Attribute
	{
		std::string_view Name;
		std::string_view Value;
		std::string_view NameSpace;
		std::string_view RawValue;
	};

	class AttributeIterator
	{
		using Iterator = std::string_view::const_iterator;

		StateEngine engine;
		NameSpaceManager const * manager {};
		Iterator ptr;
		Iterator end;
		std::optional<Iterator> currentPtr;

		/////////// working data
		Utils::PtrPair attributeName;
		Utils::PtrPair attributeValue;
		///////////

	public:
		AttributeIterator(NameSpaceManager const * manager, const Iterator begin, const Iterator end)
			: engine(State::AttributeSpace)
			, manager(manager)
			, ptr(begin)
			, end(end)
		{
			if (begin != end)
			{
				Advance();
			}
		}

		AttributeIterator() = default;

		bool operator==(AttributeIterator const & other) const
		{
			return currentPtr == other.currentPtr;
		}

		bool operator!=(AttributeIterator const & it) const
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
			auto const qName = Utils::ToStringView(attributeName);
			auto const value = Utils::ToStringView(attributeValue);
			auto const rawValue = Utils::ToStringView({attributeName.first, attributeValue.second + 1});

			if (manager != nullptr)
			{
				auto [name, nameSpace] = manager->Normalise(qName);
				return {name, value, nameSpace, rawValue};
			}
			return {qName, value, {}, rawValue};
		}

	private:
		[[noreturn]] void IllegalCharacter(char const c) const
		{
			std::ostringstream s;
			s << "Illegal character: '" << *ptr << "' (0x" << std::hex << static_cast<unsigned>(c) << ')';
			throw std::runtime_error(s.str());
		}

		void Advance()
		{
			attributeName = attributeValue = {};

			if (ptr != end)
			{
				currentPtr = ptr;
			}

			if (!currentPtr.has_value())
			{
				throw std::runtime_error("++end");
			}

			for (;;)
			{
				if (ptr == end)
				{
					currentPtr = {};
					// verify state == ?
					return;
				}

				auto const oldPtr = ptr;
				auto const oldState = engine.GetState();
				auto const newState = engine.Push(*ptr);
				if (newState == State::Error)
				{
					IllegalCharacter(*ptr);
				}
				++ptr;

				if (newState != oldState)
				{
					if (newState == State::AttributeEntity || oldState == State::AttributeEntity)
					{
						continue; // currently ignoring entities, receiver needs to decode
					}

					switch (oldState)
					{
						case State::AttributeName:
						{
							attributeName.second = oldPtr;
							break;
						}

						case State::AttributeValue:
						{
							attributeValue.second = oldPtr;
							if (manager == nullptr || !NameSpaceManager::IsDeclaration(Utils::ToStringView(attributeName)))
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
						case State::AttributeName:
						{
							attributeName.first = oldPtr;
							break;
						}

						case State::AttributeValue:
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
