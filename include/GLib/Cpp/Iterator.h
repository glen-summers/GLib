#pragma once

#include "StateEngine.h"

#include <iterator>
#include <sstream>

namespace GLib::Cpp
{
	using Fragment = std::pair<State, std::string_view>;

	class Iterator
	{
		StateEngine engine;

		const char * ptr {};
		const char * end {};
		const char * lastPtr {};
		unsigned lineNumber {1};

		Fragment fragment;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = Fragment;
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

			Iterator() = default;

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

			const Fragment & operator*() const
			{
				return fragment;
			}

			const Fragment * operator->() const
			{
				return &fragment;
			}

	private:
		[[noreturn]] static void IllegalCharacter(char c, unsigned int lineNumber, State state)
		{
			std::ostringstream s;
			s << "Illegal character: '" << c << "' (0x" << std::hex << static_cast<unsigned>(c)
				<< ") at line " << std::dec << lineNumber << " state " << static_cast<int>(state);
			throw std::runtime_error(s.str());
		}

		bool Set(State state, const char * yieldValue)
		{
			bool ret = false;
			auto size = static_cast<size_t>(yieldValue-lastPtr);
			if (size != 0)
			{
				fragment = {state, { lastPtr, size }};
				ret = true;
			}
			lastPtr = yieldValue;
			return ret;
		}

		void Close(State lastState)
		{
			if (lastState != State::None)
			{
				auto endState = engine.Push('\n');
				if (endState != State::None && endState!=State::WhiteSpace)
				{
					std::ostringstream s;
					s << "Termination error, State: " << static_cast<int>(endState);
					throw std::runtime_error(s.str());
				}
			}

			if (!Set(lastState, ptr))
			{
				lastPtr = nullptr;
			}
		}

		void Advance()
		{
			if (lastPtr == nullptr)
			{
				throw std::runtime_error("++end");
			}

			for (;;)
			{
				const auto oldState = engine.GetState();

				State newState;

				if (ptr != end)
				{
					char c = *ptr++; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) todo use std::span
					newState = engine.Push(c);
					if (newState == State::Error)
					{
						IllegalCharacter(c, lineNumber, newState);
					}
					if (c== '\n')
					{
						++lineNumber;
					}
					if (newState == oldState)
					{
						continue;
					}
				}
				else
				{
					return Close(oldState);
				}

				if (newState == State::None)
				{
					switch (oldState)
					{
						case State::CommentAsterix:
						{
							if (Set(State::CommentBlock, ptr))
							{
								return;
							}
							break;
						}
						case State::CommentLine:
						case State::String:
						case State::RawString:
						case State::CharacterLiteral:
						{
							if (Set(oldState, ptr))
							{
								return;
							}
							break;
						}
						default:;
					}
				}

				switch (oldState)
				{
					case State::WhiteSpace:
					case State::CommentLine:
					case State::Directive:
					case State::Code:
					{
						if (Set(oldState, ptr-1)) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) todo use std::span
						{
							return;
						}
						break;
					}

					case State::SystemInclude:
					{
						if (Set(oldState, ptr)) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) todo use std::span
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