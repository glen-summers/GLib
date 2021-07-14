#pragma once

#include "StateEngine.h"

#include <iterator>
#include <optional>
#include <sstream>
#include <string_view>

namespace GLib::Cpp
{
	inline std::ostream & operator<<(std::ostream & str, State s)
	{
		// clang-format off
		static constexpr std::array<std::string_view, static_cast<unsigned int>(State::Count)> stateNames
		{
			"Error",
			"None",
			"WhiteSpace",
			"CommentStart",
			"CommentLine",
			"Continuation",
			"CommentBlock",
			"CommentAsterix",
			"Directive",
			"String",
			"RawStringPrefix",
			"RawString",
			"Code",
			"CharacterLiteral",
		};
		// clang-format on
		return str << stateNames.at(static_cast<unsigned int>(s));
	}

	using Fragment = std::pair<State, std::string_view>;

	class Iterator
	{
		static constexpr char MinPrintable = 0x20;

		StateEngine engine;

		std::string_view::const_iterator ptr;
		std::string_view::const_iterator end;
		std::optional<std::string_view::const_iterator> lastPtr;
		unsigned lineNumber {1};
		unsigned startLineNumber {};

		Fragment fragment;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = Fragment;
		using difference_type = void;
		using pointer = void;
		using reference = void;

		Iterator(std::string_view::const_iterator begin, std::string_view::const_iterator end)
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
		[[noreturn]] static void IllegalCharacter(char c, unsigned int lineNumber, State state, unsigned int startLine)
		{
			std::ostringstream s;
			s << "Illegal character: ";
			if (c >= MinPrintable)
			{
				s << '\'' << c << "' ";
			}

			s << "(0x" << std::hex << static_cast<unsigned>(c) << std::dec << ") at line: " << lineNumber << ", state: " << state;

			if (startLine != lineNumber)
			{
				s << ", StartLine: " << startLine;
			}

			throw std::runtime_error(s.str());
		}

		bool Set(State state, std::string_view::const_iterator yieldValue)
		{
			bool ret = false;
			if (yieldValue != lastPtr)
			{
				// c++17 version
				fragment = {state, {&(*lastPtr)[0], static_cast<size_t>(yieldValue - *lastPtr)}};

				// c++20 version
				// fragment = {state, {*lastPtr, yieldValue}}; // current clang tidy gets: error G5C7C4CC9: no viable overloaded '='

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
				if (endState != State::None && endState != State::WhiteSpace)
				{
					std::ostringstream s;
					s << "Termination error, State: " << endState << ", StartLine: " << startLineNumber;
					throw std::runtime_error(s.str());
				}
			}

			if (!Set(lastState, ptr))
			{
				lastPtr = {};
			}
		}

		void Advance()
		{
			if (!lastPtr.has_value())
			{
				throw std::runtime_error("++end");
			}

			for (;;)
			{
				const auto oldState = engine.GetState();

				State newState;

				if (ptr != end)
				{
					char c = *ptr++;
					newState = engine.Push(c);
					if (newState == State::Error)
					{
						IllegalCharacter(c, lineNumber, oldState, startLineNumber);
					}
					if (c == '\n')
					{
						++lineNumber;
					}
					if (newState == oldState)
					{
						continue;
					}
					startLineNumber = lineNumber;
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
						if (Set(oldState, ptr - 1))
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
		std::string_view value;

	public:
		Holder(std::string_view value)
			: value(value)
		{}

		Iterator begin() const
		{
			return {value.cbegin(), value.cend()};
		}

		Iterator end() const
		{
			(void) this;
			return {};
		}
	};
}