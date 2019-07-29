#pragma once

#include <cctype>
#include <array>

namespace GLib::Xml
{
	enum class State : int
	{
		Error,
		Start,
		DocTypeDecl,
		ElementStart,
		ElementEnd,
		ElementEndName,
		ElementEndSpace,
		ElementName,
		EmptyElement,
		ElementAttributeSpace,
		ElementAttributeName,
		ElementAttributeNameSpace,
		ElementAttributeValueStart,
		ElementAttributeValueQuote,
		ElementAttributeValueSingleQuote,
		ElementAttributeQuoteEnd,
		Text,

		Bang,
		CommentStartDash,
		Comment,
		CommentEndDash,
		CommentEnd,

		XmlDeclaration,

		CDataName,
		CDataValue,
		CDataEnd1,
		CDataEnd2,

		Count
	};

	class StateEngine
	{
		static constexpr char LeftAngleBracket = '<';
		static constexpr char RightAngleBracket = '>';
		static constexpr char ForwardSlash = '/';
		static constexpr char Equals = '=';
		static constexpr char DoubleQuote = '"';
		static constexpr char SingleQuote = '\'';
		static constexpr char Colon = ':';
		static constexpr char Underscore = '_';
		static constexpr char FullStop = '.';
		static constexpr char Hyphen = '-';
		static constexpr char Exclamation = '!';
		static constexpr char Dash = '-';
		static constexpr char QuestionMark = '?';
		static constexpr char LeftSquareBracket = '[';
		static constexpr char RightSquareBracket = ']';
		static constexpr char Ampersand = '&';

		using StateFunction = State (StateEngine::*)(char);

		// use Phase : Prolog, Document, End
		// could also manage depth here to dtermine end
		// but try in iterator first?
		State state;
		bool isProlog { true };
		bool hasDocTypeDecl {};
		bool hasContent {};
		StateFunction stateFunction;

	public:
		StateEngine(State state = State::Start)
		  : state(state)
		  , stateFunction(stateFunctions.at(static_cast<int>(state)))
		{}

		State GetState() const
		{
			return state;
		}

		bool HasRootElement() const
		{
			return !isProlog;
		}

		State Push(char value)
		{
			SetState((this->*stateFunction)(value));
			return state;
		}

	private:
		static bool IsContinuation(char c)
		{
			return (c & 0x80) != 0;
		}

		static bool IsWhiteSpace(char c)
		{
			return !IsContinuation(c) && std::isspace(c) != 0;
		}

		static bool IsNameStart(char c)
		{
			return IsContinuation(c) || std::isalpha(c) != 0 || c == Colon || c == Underscore; // check docs
		}

		static bool IsName(char c)
		{
			return IsNameStart(c) || std::isdigit(c) != 0 || c == FullStop || c == Hyphen; // check docs
		}

		static bool IsAllowedTextCharacter(char c)
		{
			return c != LeftAngleBracket && c != Ampersand;
		}

		void SetState(Xml::State newState)
		{
			if (newState != state)
			{
				state = newState;
				stateFunction = stateFunctions.at(static_cast<int>(state));
			}
		}

		////////////////////////
		// state functions
		Xml::State Error(char c)
		{
			(void)c;
			return state;
		}

		Xml::State Start(char c)
		{
			if (c == LeftAngleBracket)
			{
				return State::ElementStart;
			}
			if (IsWhiteSpace(c))
			{
				hasContent = true;
				return state;
			}
			if (!isProlog && IsAllowedTextCharacter(c))
			{
				return State::Text;
			}
			return State::Error;
		}

		Xml::State DocTypeDecl(char c)
		{
			if (IsName(c) || IsWhiteSpace(c))
			{
				return state;
			}
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			return state;
		}

		Xml::State ElementStart(char c)
		{
			if (IsNameStart(c))
			{
				isProlog = false;
				hasContent = true;
				return State::ElementName;
			}
			if (c == ForwardSlash)
			{
				return State::ElementEnd;
			}
			if (c == Exclamation)
			{
				hasContent = true;
				return State::Bang;
			}
			if (c == QuestionMark && !hasContent)
			{
				return State::XmlDeclaration;
			}
			return State::Error;
		}

		Xml::State ElementEnd(char c)
		{
			if (IsNameStart(c))
			{
				return State::ElementEndName;
			}
			return State::Error;
		}

		Xml::State ElementEndName(char c)
		{
			if (IsName(c))
			{
				return state;
			}
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			if (IsWhiteSpace(c))
			{
				return State::ElementEndSpace;
			}
			return State::Error;
		}

		Xml::State ElementEndSpace(char c)
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		Xml::State ElementName(char c)
		{
			if (IsName(c))
			{
				return state;
			}
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			if (c == ForwardSlash)
			{
				return State::EmptyElement;
			}
			if (IsWhiteSpace(c))
			{
				return State::ElementAttributeSpace;
			}
			return State::Error;
		}

		Xml::State EmptyElement(char c)
		{
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		Xml::State ElementAttributeSpace(char c)
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			if (c == ForwardSlash)
			{
				return State::EmptyElement;
			}
			if (IsNameStart(c))
			{
				return State::ElementAttributeName;
			}
			return State::Error;
		}

		Xml::State ElementAttributeName(char c)
		{
			if (IsName(c))
			{
				return state;
			}
			if (IsWhiteSpace(c))
			{
				return State::ElementAttributeNameSpace;
			}
			if (c == Equals)
			{
				return State::ElementAttributeValueStart;
			}
			return State::Error;
		}

		Xml::State ElementAttributeNameSpace(char c)
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == Equals)
			{
				return State::ElementAttributeValueStart;
			}
			return State::Error;
		}

		Xml::State ElementAttributeValueStart(char c)
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == DoubleQuote)
			{
				return State::ElementAttributeValueQuote;
			}
			if (c == SingleQuote)
			{
				return State::ElementAttributeValueSingleQuote;
			}
			return State::Error;
		}

		Xml::State ElementAttributeValueQuote(char c)
		{
			if (c == DoubleQuote)
			{
				return State::ElementAttributeQuoteEnd;
			}
			if (IsAllowedTextCharacter(c))
			{
				return state;
			}
			return State::Error;
		}

		Xml::State ElementAttributeValueSingleQuote(char c)
		{
			if (c == SingleQuote)
			{
				return State::ElementAttributeQuoteEnd;
			}
			if (IsAllowedTextCharacter(c))
			{
				return state;
			}
			return State::Error;
		}

		Xml::State ElementAttributeQuoteEnd(char c)
		{
			if (IsWhiteSpace(c))
			{
				return State::ElementAttributeSpace;
			}
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			if (c == ForwardSlash)
			{
				return State::EmptyElement;
			}
			return State::Error;
		}

		Xml::State Text(char c)
		{
			if (c == LeftAngleBracket)
			{
				return State::ElementStart;
			}
			if (IsAllowedTextCharacter(c))
			{
				return state;
			}
			return State::Error;
		}

		Xml::State Bang(char c)
		{
			if (c == Dash)
			{
				return State::CommentStartDash;
			}
			if (c == LeftSquareBracket)
			{
				return State::CDataName;
			}
			if (isProlog && !hasDocTypeDecl && IsNameStart(c))
			{
				hasDocTypeDecl = true;
				return State::DocTypeDecl;
			}
			return State::Error;
		}

		Xml::State CommentStartDash(char c)
		{
			if (c == Dash)
			{
				return State::Comment;
			}
			return State::Error;
		}

		Xml::State Comment(char c)
		{
			if (c == Dash)
			{
				return State::CommentEndDash;
			}
			return state;
		}

		Xml::State CommentEndDash(char c)
		{
			if (c == Dash)
			{
				return State::CommentEnd;
			}
			return State::Comment;
		}

		Xml::State CommentEnd(char c)
		{
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		Xml::State XmlDeclaration(char c)
		{
			if (c == QuestionMark)
			{
				return Xml::State::EmptyElement;
			}
			return state;
		}

		Xml::State CDataName(char c)
		{
			if (c == LeftSquareBracket)
			{
				return Xml::State::CDataValue;
			}
			return state;
		}

		Xml::State CDataValue(char c)
		{
			if (c == RightSquareBracket)
			{
				return Xml::State::CDataEnd1;
			}
			return state;
		}

		Xml::State CDataEnd1(char c)
		{
			if (c == RightSquareBracket)
			{
				return Xml::State::CDataEnd2;
			}
			return Xml::State::CDataValue;
		}

		Xml::State CDataEnd2(char c)
		{
			if (c == RightAngleBracket)
			{
				return Xml::State::Start;
			}
			return Xml::State::CDataValue;
		}
		// state functions
		//////////////////////

		// must be enum order
		inline static std::array<StateFunction, static_cast<int>(State::Count)> stateFunctions =
		{
			&StateEngine::Error,
			&StateEngine::Start,
			&StateEngine::DocTypeDecl,
			&StateEngine::ElementStart,
			&StateEngine::ElementEnd,
			&StateEngine::ElementEndName,
			&StateEngine::ElementEndSpace,
			&StateEngine::ElementName,
			&StateEngine::EmptyElement,
			&StateEngine::ElementAttributeSpace,
			&StateEngine::ElementAttributeName,
			&StateEngine::ElementAttributeNameSpace,
			&StateEngine::ElementAttributeValueStart,
			&StateEngine::ElementAttributeValueQuote,
			&StateEngine::ElementAttributeValueSingleQuote,
			&StateEngine::ElementAttributeQuoteEnd,
			&StateEngine::Text,

			&StateEngine::Bang,
			&StateEngine::CommentStartDash,
			&StateEngine::Comment,
			&StateEngine::CommentEndDash,
			&StateEngine::CommentEnd,

			&StateEngine::XmlDeclaration,

			&StateEngine::CDataName,
			&StateEngine::CDataValue,
			&StateEngine::CDataEnd1,
			&StateEngine::CDataEnd2,
		};
	};
}