#pragma once

#include "StateEngine.h"

#include <iterator>
#include <optional>
#include <sstream>

namespace GLib::Cpp
{
	inline std::ostream & operator<<(std::ostream & str, State const state)
	{
		static constexpr std::array<std::string_view, static_cast<unsigned int>(State::Count)> stateNames {
			"Error",					 "None",			"WhiteSpace", "CommentStart",		 "CommentLine", "Continuation", "CommentBlock",
			"CommentAsterisk", "Directive", "String",			"RawStringPrefix", "RawString",		"Code",					"CharacterLiteral",
		};

		return str << stateNames.at(static_cast<unsigned int>(state));
	}

	using Fragment = std::pair<State, std::string_view>;

	class Iterator
	{
		static constexpr char minPrintable = 0x20;

		StateEngine engine;

		std::string_view::const_iterator ptr;
		std::string_view::const_iterator const end;
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

		Iterator(std::string_view::const_iterator const begin, std::string_view::const_iterator const end, bool const emitWhitespace)
			: engine(emitWhitespace)
			, ptr(begin)
			, end(end)
			, lastPtr(begin)
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

		Fragment const & operator*() const
		{
			return fragment;
		}

		Fragment const * operator->() const
		{
			return &fragment;
		}

	private:
		[[noreturn]] static void IllegalCharacter(char const chr, unsigned int const lineNumber, State const state, unsigned int const startLine)
		{
			std::ostringstream stm;
			stm << "Illegal character: ";
			if (chr >= minPrintable)
			{
				stm << '\'' << chr << "' ";
			}

			stm << "(0x" << std::hex << static_cast<unsigned>(chr) << std::dec << ") at line: " << lineNumber << ", state: " << state;

			if (startLine != lineNumber)
			{
				stm << ", StartLine: " << startLine;
			}

			throw std::runtime_error(stm.str());
		}

		bool Set(State state, std::string_view::const_iterator const yieldValue)
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

		void Close(State const lastState)
		{
			if (lastState != State::None)
			{
				auto const endState = engine.Push('\n');
				if (endState != State::None && endState != State::WhiteSpace)
				{
					std::ostringstream stm;
					stm << "Termination error, State: " << endState << ", StartLine: " << startLineNumber;
					throw std::runtime_error(stm.str());
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
				auto const oldState = engine.GetState();

				State newState {};

				if (ptr != end)
				{
					char const chr = *ptr++;
					newState = engine.Push(chr);
					if (newState == State::Error)
					{
						IllegalCharacter(chr, lineNumber, oldState, startLineNumber);
					}
					if (chr == '\n')
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
		std::string_view const value;
		bool const emitWhitespace;

	public:
		explicit Holder(std::string_view const value, bool const emitWhitespace = true)
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