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

		bool const emitWhiteSpace {};
		State state {};
		StateFunction stateFunction {};
		char lastChar {};

		mutable std::string rawStringPrefix;
		mutable size_t matchCount {};
		mutable State continuationState {};
		mutable bool stringEscape {};

	public:
		StateEngine() = default;

		explicit StateEngine(bool const emitWhiteSpace)
			: emitWhiteSpace {emitWhiteSpace}
			, state {State::None}
			, stateFunction {&StateEngine::None}
		{
			rawStringPrefix.reserve(maxPrefixSize);
		}

		State Push(char const value)
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
		static bool IsContinuation(char const chr)
		{
			return (static_cast<EnumType>(chr) & continuationMask) != 0;
		}

		bool IsWhiteSpace(char const chr) const
		{
			return emitWhiteSpace && !IsContinuation(chr) && std::isspace(chr) != 0;
		}

		void SetState(State const newState)
		{
			if (newState != state)
			{
				state = newState;
				stateFunction = stateFunctions.at(static_cast<EnumType>(state));
			}
		}

		void SetContinue(State const newState) const
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

		State Error(char const chr) const
		{
			static_cast<void>(chr);
			return state;
		}

		State None(char const chr) const
		{
			if (chr == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (chr == hash)
			{
				return State::Directive;
			}

			if (chr == doubleQuote)
			{
				return State::String;
			}

			if (chr == singleQuote)
			{
				return State::CharacterLiteral;
			}

			if (IsWhiteSpace(chr))
			{
				return emitWhiteSpace ? State::WhiteSpace : state;
			}

			return State::Code;
		}

		State WhiteSpace(char const chr) const
		{
			if (chr == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (chr == hash)
			{
				return State::Directive;
			}

			if (chr == doubleQuote)
			{
				return State::String;
			}

			if (chr == singleQuote)
			{
				return State::CharacterLiteral;
			}

			if (chr == newLine)
			{
				return State::None;
			}

			if (!IsWhiteSpace(chr))
			{
				return State::Code;
			}

			return state;
		}

		State CommentStart(char const chr) const
		{
			if (chr == forwardSlash)
			{
				return State::CommentLine;
			}
			if (chr == asterisk)
			{
				return State::CommentBlock;
			}
			return Continue();
		}

		State CommentLine(char const chr) const
		{
			if (chr == newLine && lastChar != backSlash)
			{
				return State::None;
			}

			return state;
		}

		State Continuation(char /*chr*/) const
		{
			return Continue();
		}

		State CommentBlock(char const chr) const
		{
			if (chr == asterisk)
			{
				return State::CommentAsterisk;
			}
			return state;
		}

		State CommentAsterisk(char const chr) const
		{
			if (chr == forwardSlash)
			{
				return State::None;
			}
			if (chr != asterisk)
			{
				return State::CommentBlock;
			}
			return state;
		}

		State Directive(char const chr) const
		{
			if (chr == newLine && lastChar != backSlash)
			{
				return State::None;
			}

			if (chr == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			return state;
		}

		State String(char const chr) const
		{
			if (chr == backSlash)
			{
				stringEscape = !stringEscape;
				return state;
			}

			if (stringEscape)
			{
				stringEscape = {};
				return state;
			}

			if (chr == doubleQuote)
			{
				return State::None;
			}

			return state;
		}

		State RawStringPrefix(char const chr) const
		{
			if (chr == openParenthesis)
			{
				matchCount = 0;
				return State::RawString;
			}

			if (IsWhiteSpace(chr) || chr == closeParenthesis || chr == backSlash)
			{
				return State::Error;
			}

			if (rawStringPrefix.size() == maxPrefixSize)
			{
				return State::Error;
			}

			rawStringPrefix += chr;
			return state;
		}

		State RawString(char const chr) const
		{
			if (chr == closeParenthesis)
			{
				if (matchCount != 0)
				{
					matchCount = 1;
					return state;
				}
				++matchCount;
				return state;
			}

			if (matchCount != 0 && matchCount - 1 == rawStringPrefix.size() && chr == doubleQuote)
			{
				return State::None;
			}

			if (matchCount != 0 && matchCount - 1 < rawStringPrefix.size() && chr == rawStringPrefix[matchCount - 1])
			{
				++matchCount;
			}
			else
			{
				matchCount = 0;
			}

			return state;
		}

		State Code(char const chr) const
		{
			if (emitWhiteSpace && IsWhiteSpace(chr))
			{
				return State::WhiteSpace;
			}

			if (chr == forwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (chr == doubleQuote)
			{
				if (lastChar == 'R')
				{
					rawStringPrefix.clear();
					SetContinue(state);
					return State::RawStringPrefix;
				}
				return State::String;
			}

			if (chr == singleQuote && std::isxdigit(lastChar) == 0)
			{
				return State::CharacterLiteral;
			}

			if (chr == '\n')
			{
				return State::None;
			}

			return state;
		}

		State CharacterLiteral(char const chr) const
		{
			if (chr == backSlash)
			{
				stringEscape = !stringEscape;
				return state;
			}

			if (chr == newLine)
			{
				return State::Error;
			}

			if (stringEscape)
			{
				stringEscape = {};
				return state;
			}

			if (chr == singleQuote)
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