#pragma once

#include <array>
#include <cctype>
#include <string>
#include <utility>

namespace GLib::Cpp
{
	enum class State : unsigned int
	{
		Error,
		None,							// /:CommentStart, #:Directive, R":RawStringPrefix, ":String, ':CharacterLiteral, WS:WhiteSpace, Else:Code
		WhiteSpace,				// /:CommentStart, #:Directive, R":RawStringPrefix, ":String, ':CharacterLiteral, NL:None, !WS:Code
		CommentStart,			// /:CommentLine,  *:CommentBlock
		CommentLine,			// \:Continuation, NL:None   *sets return state*
		Continuation,			// NL: <return continue state>, else if /:self? !\:CommentLine?
		CommentBlock,			// *:CommentAsterix
		CommentAsterix,		// /:None, Else:CommentBlock
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
		static constexpr char ForwardSlash = '/';
		static constexpr char BackSlash = '\\';
		static constexpr char Asterix = '*';
		static constexpr char NewLine = '\n';
		static constexpr char Hash = '#';
		static constexpr char DoubleQuote = '"';
		static constexpr char OpenParenthesis = '(';
		static constexpr char CloseParenthesis = ')';
		static constexpr char RawStringStart = 'R';
		static constexpr char OpenAngleBracket = '<';
		static constexpr char CloseAngleBracket = '>';
		static constexpr char SingleQuote = '\'';

		static constexpr auto ContinuationMask = 0x80U;

		static constexpr auto MaxPrefixSize = 16;

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
			rawStringPrefix.reserve(MaxPrefixSize);
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
			return (static_cast<unsigned char>(c) & ContinuationMask) != 0;
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
				stateFunction = stateFunctions.at(static_cast<int>(state));
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
			(void) c;
			return state;
		}

		State None(char c) const
		{
			if (c == ForwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (c == Hash)
			{
				return State::Directive;
			}

			if (c == DoubleQuote)
			{
				return State::String;
			}

			if (c == SingleQuote)
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
			if (c == ForwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (c == Hash)
			{
				return State::Directive;
			}

			if (c == DoubleQuote)
			{
				return State::String;
			}

			if (c == SingleQuote)
			{
				return State::CharacterLiteral;
			}

			if (c == NewLine)
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
			if (c == ForwardSlash)
			{
				return State::CommentLine;
			}
			if (c == Asterix)
			{
				return State::CommentBlock;
			}
			return Continue();
		}

		State CommentLine(char c) const
		{
			if (c == NewLine && lastChar != BackSlash)
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
			if (c == Asterix)
			{
				return State::CommentAsterix;
			}
			return state;
		}

		State CommentAsterix(char c) const
		{
			if (c == ForwardSlash)
			{
				return State::None;
			}
			if (c != Asterix)
			{
				return State::CommentBlock;
			}
			return state;
		}

		State Directive(char c) const
		{
			if (c == NewLine && lastChar != BackSlash)
			{
				return State::None;
			}

			if (c == ForwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			return state;
		}

		State String(char c) const
		{
			if (c == BackSlash)
			{
				stringEscape = !stringEscape;
				return state;
			}

			if (stringEscape)
			{
				stringEscape = {};
				return state;
			}

			if (c == DoubleQuote)
			{
				return State::None;
			}

			return state;
		}

		State RawStringPrefix(char c) const
		{
			if (c == OpenParenthesis)
			{
				matchCount = 0;
				return State::RawString;
			}

			if (IsWhiteSpace(c) || c == CloseParenthesis || c == BackSlash)
			{
				return State::Error;
			}

			if (rawStringPrefix.size() == MaxPrefixSize)
			{
				return State::Error;
			}

			rawStringPrefix += c;
			return state;
		}

		State RawString(char c) const
		{
			if (c == CloseParenthesis)
			{
				if (matchCount != 0)
				{
					matchCount = 1;
					return state;
				}
				++matchCount;
				return state;
			}

			if (matchCount != 0 && matchCount - 1 == rawStringPrefix.size() && c == DoubleQuote)
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

			if (c == ForwardSlash)
			{
				SetContinue(state);
				return State::CommentStart;
			}

			if (c == DoubleQuote)
			{
				if (lastChar == 'R')
				{
					rawStringPrefix.clear();
					SetContinue(state);
					return State::RawStringPrefix;
				}
				return State::String;
			}

			if (c == SingleQuote && (std::isxdigit(lastChar) == 0))
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
			if (c == BackSlash)
			{
				stringEscape = !stringEscape;
				return state;
			}

			if (c == NewLine)
			{
				return State::Error;
			}

			if (stringEscape)
			{
				stringEscape = {};
				return state;
			}

			if (c == SingleQuote)
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
			&StateEngine::CommentAsterix,
			&StateEngine::Directive,
			&StateEngine::String,
			&StateEngine::RawStringPrefix,
			&StateEngine::RawString,
			&StateEngine::Code,
			&StateEngine::CharacterLiteral,
		};
	};
}