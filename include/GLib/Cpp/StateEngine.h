#pragma once

#include <array>
#include <cctype>
#include <string>
#include <utility>

namespace GLib::Cpp
{
	using EnumType = uint8_t;

	enum class State : EnumType
	{
		Error,
		None,							// /:CommentStart, #:Directive, R":RawStringPrefix, ":String, ':CharacterLiteral, WS:WhiteSpace, Else:Code
		WhiteSpace,				// /:CommentStart, #:Directive, R":RawStringPrefix, ":String, ':CharacterLiteral, NL:None, !WS:Code
		CommentStart,			// /:CommentLine,  *:CommentBlock
		CommentLine,			// \:Continuation, NL:None   *sets return state*
		Continuation,			// NL: <return continue state>, else if /:self? !\:CommentLine?
		CommentBlock,			// *:CommentAsterisk
		CommentAsterisk,	// /:None, Else:CommentBlock
		Directive,				// NL:None, /:CommentStart, \:Continuation, *sets return state*
		String,						// ":None, \:Continuation  *sets return state*
		RawStringPrefix,	// (: RawString
		RawString,				// ): None  not continue state?
		Code,							// WS:Whitespace, ":String, R":RawStringPrefix, ':CharacterLiteral  *sets return state*
		CharacterLiteral, // ':None

		Count
	};

	class StateEngine
	{
		static constexpr char forwardSlash = '/';
		static constexpr char backSlash = '\\';
		static constexpr char asterisk = '*';
		static constexpr char newLine = '\n';
		static constexpr char hash = '#';
		static constexpr char doubleQuote = '"';
		static constexpr char openParenthesis = '(';
		static constexpr char closeParenthesis = ')';
		static constexpr char rawStringStart = 'R';
		static constexpr char openAngleBracket = '<';
		static constexpr char closeAngleBracket = '>';
		static constexpr char singleQuote = '\'';

		static constexpr EnumType continuationMask = 0x80U;

		static constexpr auto maxPrefixSize = 16;

		using StateFunction = State (StateEngine::*)(char) const;

		bool emitWhiteSpace {};
		State state {};
		StateFunction stateFunction {};
		char lastChar {};

		mutable std::string rawStringPrefix;
		mutable size_t matchCount {};
		mutable State continuationState {};
		mutable bool stringEscape {};

	public:
		StateEngine() = default;

		explicit StateEngine(bool emitWhiteSpace)
			: emitWhiteSpace {emitWhiteSpace}
			, state {State::None}
			, stateFunction {&StateEngine::None}
		{
			rawStringPrefix.reserve(maxPrefixSize);
		}

		State Push(char value)
		{
			SetState((this->*stateFunction)(value));
			lastChar = value;
			return state;
		}

		State GetState() const
		{
			return state;
		}

	private:
		static bool IsContinuation(char c)
		{
			return (static_cast<EnumType>(c) & continuationMask) != 0;
		}

		bool IsWhiteSpace(char c) const
		{
			return emitWhiteSpace && !IsContinuation(c) && std::isspace(c) != 0;
		}

		void SetState(State newState)
		{
			if (newState != state)
			{
				state = newState;
				stateFunction = stateFunctions.at(static_cast<EnumType>(state));
			}
		}

		void SetContinue(State newState) const
		{
			// for this to work need to unset continue states when they are invalidated
			/*if (continuationState != State::Error)
			{
				throw std::logic_error("Continue state already set");
			}*/
			continuationState = newState;
		}

		State Continue() const
		{
			return std::exchange(continuationState, State::Error);
		}

		State Error(char c) const
		{
			static_cast<void>(c);
			return state;
		}

		State None(char c) const
		{
			if (c == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (c == hash)
			{
				return State::Directive;
			}

			if (c == doubleQuote)
			{
				return State::String;
			}

			if (c == singleQuote)
			{
				return State::CharacterLiteral;
			}

			if (IsWhiteSpace(c))
			{
				return emitWhiteSpace ? State::WhiteSpace : state;
			}

			return State::Code;
		}

		State WhiteSpace(char c) const
		{
			if (c == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (c == hash)
			{
				return State::Directive;
			}

			if (c == doubleQuote)
			{
				return State::String;
			}

			if (c == singleQuote)
			{
				return State::CharacterLiteral;
			}

			if (c == newLine)
			{
				return State::None;
			}

			if (!IsWhiteSpace(c))
			{
				return State::Code;
			}

			return state;
		}

		State CommentStart(char c) const
		{
			if (c == forwardSlash)
			{
				return State::CommentLine;
			}
			if (c == asterisk)
			{
				return State::CommentBlock;
			}
			return Continue();
		}

		State CommentLine(char c) const
		{
			if (c == newLine && lastChar != backSlash)
			{
				return State::None;
			}

			return state;
		}

		State Continuation(char /*c*/) const
		{
			return Continue();
		}

		State CommentBlock(char c) const
		{
			if (c == asterisk)
			{
				return State::CommentAsterisk;
			}
			return state;
		}

		State CommentAsterisk(char c) const
		{
			if (c == forwardSlash)
			{
				return State::None;
			}
			if (c != asterisk)
			{
				return State::CommentBlock;
			}
			return state;
		}

		State Directive(char c) const
		{
			if (c == newLine && lastChar != backSlash)
			{
				return State::None;
			}

			if (c == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			return state;
		}

		State String(char c) const
		{
			if (c == backSlash)
			{
				stringEscape = !stringEscape;
				return state;
			}

			if (stringEscape)
			{
				stringEscape = {};
				return state;
			}

			if (c == doubleQuote)
			{
				return State::None;
			}

			return state;
		}

		State RawStringPrefix(char c) const
		{
			if (c == openParenthesis)
			{
				matchCount = 0;
				return State::RawString;
			}

			if (IsWhiteSpace(c) || c == closeParenthesis || c == backSlash)
			{
				return State::Error;
			}

			if (rawStringPrefix.size() == maxPrefixSize)
			{
				return State::Error;
			}

			rawStringPrefix += c;
			return state;
		}

		State RawString(char c) const
		{
			if (c == closeParenthesis)
			{
				if (matchCount != 0)
				{
					matchCount = 1;
					return state;
				}
				++matchCount;
				return state;
			}

			if (matchCount != 0 && matchCount - 1 == rawStringPrefix.size() && c == doubleQuote)
			{
				return State::None;
			}

			if (matchCount != 0 && matchCount - 1 < rawStringPrefix.size() && c == rawStringPrefix[matchCount - 1])
			{
				++matchCount;
			}
			else
			{
				matchCount = 0;
			}

			return state;
		}

		State Code(char c) const
		{
			if (emitWhiteSpace && IsWhiteSpace(c))
			{
				return State::WhiteSpace;
			}

			if (c == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (c == doubleQuote)
			{
				if (lastChar == 'R')
				{
					rawStringPrefix.clear();
					SetContinue(state);
					return State::RawStringPrefix;
				}
				return State::String;
			}

			if (c == singleQuote && std::isxdigit(lastChar) == 0)
			{
				return State::CharacterLiteral;
			}

			if (c == '\n')
			{
				return State::None;
			}

			return state;
		}

		State CharacterLiteral(char c) const
		{
			if (c == backSlash)
			{
				stringEscape = !stringEscape;
				return state;
			}

			if (c == newLine)
			{
				return State::Error;
			}

			if (stringEscape)
			{
				stringEscape = {};
				return state;
			}

			if (c == singleQuote)
			{
				return State::None;
			}

			return state;
		}

		// must be enum order
		static constexpr std::array<StateFunction, static_cast<int>(State::Count)> stateFunctions = {
			&StateEngine::Error,
			&StateEngine::None,
			&StateEngine::WhiteSpace,
			&StateEngine::CommentStart,
			&StateEngine::CommentLine,
			&StateEngine::Continuation,
			&StateEngine::CommentBlock,
			&StateEngine::CommentAsterisk,
			&StateEngine::Directive,
			&StateEngine::String,
			&StateEngine::RawStringPrefix,
			&StateEngine::RawString,
			&StateEngine::Code,
			&StateEngine::CharacterLiteral,
		};
	};
}