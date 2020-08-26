#pragma once

#include "StateEngine.h"

#include <iterator>
#include <sstream>

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

		const char * ptr {};
		const char * end {};
		const char * lastPtr {};
		unsigned lineNumber {1};
		unsigned startLineNumber {};

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

		bool Set(State state, const char * yieldValue)
		{
			bool ret = false;
			auto size = static_cast<size_t>(yieldValue - lastPtr);
			if (size != 0)
			{
				fragment = {state, {lastPtr, size}};
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
						if (Set(oldState, ptr - 1)) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) todo use std::span
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
			return Iterator {value.data(), value.size() + value.data()};
		}

		Iterator end() const
		{
			(void) this;
			return Iterator {};
		}
	};
}