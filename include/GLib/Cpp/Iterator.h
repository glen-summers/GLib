#pragma once

#include "StateEngine.h"

#include <iterator>
#include <optional>
#include <sstream>

namespace GLib::Cpp
{
	inline std::ostream & operator<<(std::ostream & str, State s)
	{
		static constexpr std::array<std::string_view, static_cast<unsigned int>(State::Count)> stateNames {
			"Error",					 "None",			"WhiteSpace", "CommentStart",		 "CommentLine", "Continuation", "CommentBlock",
			"CommentAsterisk", "Directive", "String",			"RawStringPrefix", "RawString",		"Code",					"CharacterLiteral",
		};

		return str << stateNames.at(static_cast<unsigned int>(s));
	}

	using Fragment = std::pair<State, std::string_view>;

	class Iterator
	{
		static constexpr char minPrintable = 0x20;

		StateEngine engine;

		std::string_view::const_iterator ptr;
		std::string_view::const_iterator end;
		std::optional<std::string_view::const_iterator> lastPtr;
		unsigned lineNumber {1};
		unsigned startLineNumber {};

		Fragment fragment;

	public:
		// ReSharper disable All
		using iterator_category = std::forward_iterator_tag;
		using value_type = Fragment;
		using difference_type = void;
		using pointer = void;
		using reference = void;
		// ReSharper restore All

		Iterator(std::string_view::const_iterator begin, std::string_view::const_iterator end, bool emitWhitespace)
			: engine(emitWhitespace)
			, ptr(begin)
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
			if (c >= minPrintable)
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
				fragment = {state, {*lastPtr, yieldValue}};
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

				State newState {};

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

				//
				if (newState == State::None && oldState == State::CommentAsterisk && Set(State::CommentBlock, ptr))
				{
					return;
				}

				if ((oldState == State::CommentLine || oldState == State::String || oldState == State::RawString || oldState == State::CharacterLiteral) &&
						Set(oldState, ptr))
				{
					return;
				}

				if ((oldState == State::WhiteSpace || oldState == State::CommentLine || oldState == State::Directive || oldState == State::Code) &&
						Set(oldState, ptr - 1))
				{
					return;
				}
			}
		}
	};

	class Holder
	{
		std::string_view value;
		bool emitWhitespace;

	public:
		explicit Holder(std::string_view value, bool emitWhitespace = true)
			: value(value)
			, emitWhitespace(emitWhitespace)
		{}

		[[nodiscard]] Iterator begin() const
		{
			return {value.cbegin(), value.cend(), emitWhitespace};
		}

		[[nodiscard]] Iterator end() const
		{
			static_cast<void>(this);
			return {value.cend(), value.cend(), emitWhitespace};
		}
	};
}