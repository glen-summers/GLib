#pragma once

#include <array>
#include <cctype>

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
		AttributeSpace,
		AttributeName,
		AttributeNameSpace,
		AttributeValueStart,
		AttributeValue,
		AttributeEnd,
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

		TextEntity,
		AttributeEntity,

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
		static constexpr char SemiColon = ';';
		static constexpr char Underscore = '_';
		static constexpr char FullStop = '.';
		static constexpr char Hyphen = '-';
		static constexpr char Exclamation = '!';
		static constexpr char Dash = '-';
		static constexpr char QuestionMark = '?';
		static constexpr char LeftSquareBracket = '[';
		static constexpr char RightSquareBracket = ']';
		static constexpr char Ampersand = '&';

		static constexpr auto ContinuationMask = 0x80U;

		using StateFunction = State (StateEngine::*)(char) const;

		// use Phase : Prologue, Document, End
		// could also manage depth here to determine end
		// but try in iterator first?
		State state;
		bool mutable isProlog { true };
		bool mutable hasDocTypeDecl {};
		bool mutable hasContent {};
		char mutable attributeQuoteChar {};
		StateFunction stateFunction;

	public:
		explicit StateEngine(State state = State::Start)
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
			return (static_cast<unsigned char>(c) & ContinuationMask) != 0;
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
			return c != LeftAngleBracket;
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
		Xml::State Error(char c) const
		{
			(void)c;
			return state;
		}

		Xml::State Start(char c) const
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

		Xml::State DocTypeDecl(char c) const
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

		Xml::State ElementStart(char c) const
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

		Xml::State ElementEnd(char c) const // NOLINT
		{
			if (IsNameStart(c))
			{
				return State::ElementEndName;
			}
			return State::Error;
		}

		Xml::State ElementEndName(char c) const
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

		Xml::State ElementEndSpace(char c) const
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

		Xml::State ElementName(char c) const
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
				return State::AttributeSpace;
			}
			return State::Error;
		}

		Xml::State EmptyElement(char c) const // NOLINT
		{
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		Xml::State AttributeSpace(char c) const
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
				return State::AttributeName;
			}
			return State::Error;
		}

		Xml::State AttributeName(char c) const
		{
			if (IsName(c))
			{
				return state;
			}
			if (IsWhiteSpace(c))
			{
				return State::AttributeNameSpace;
			}
			if (c == Equals)
			{
				return State::AttributeValueStart;
			}
			return State::Error;
		}

		Xml::State AttributeNameSpace(char c) const
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == Equals)
			{
				return State::AttributeValueStart;
			}
			return State::Error;
		}

		Xml::State AttributeValueStart(char c) const
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == DoubleQuote || c == SingleQuote)
			{
				attributeQuoteChar = c;
				return State::AttributeValue;
			}
			return State::Error;
		}

		Xml::State AttributeValue(char c) const // NOLINT
		{
			if (c == attributeQuoteChar)
			{
				return State::AttributeEnd;
			}
			if (c == Ampersand)
			{
				return State::AttributeEntity;
			}
			if (IsAllowedTextCharacter(c))
			{
				return state;
			}
			return State::Error;
		}

		Xml::State AttributeEnd(char c) const // NOLINT
		{
			if (IsWhiteSpace(c))
			{
				return State::AttributeSpace;
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

		Xml::State Text(char c) const
		{
			if (c == LeftAngleBracket)
			{
				return State::ElementStart;
			}
			if (c == Ampersand)
			{
				return State::TextEntity;
			}
			if (IsAllowedTextCharacter(c))
			{
				return state;
			}
			return State::Error;
		}

		Xml::State Bang(char c) const
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

		Xml::State CommentStartDash(char c) const // NOLINT
		{
			if (c == Dash)
			{
				return State::Comment;
			}
			return State::Error;
		}

		Xml::State Comment(char c) const
		{
			if (c == Dash)
			{
				return State::CommentEndDash;
			}
			return state;
		}

		Xml::State CommentEndDash(char c) const // NOLINT
		{
			if (c == Dash)
			{
				return State::CommentEnd;
			}
			return State::Comment;
		}

		Xml::State CommentEnd(char c) const // NOLINT
		{
			if (c == RightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		Xml::State XmlDeclaration(char c) const
		{
			if (c == QuestionMark)
			{
				return Xml::State::EmptyElement;
			}
			return state;
		}

		Xml::State CDataName(char c) const
		{
			if (c == LeftSquareBracket)
			{
				return Xml::State::CDataValue;
			}
			return state;
		}

		Xml::State CDataValue(char c) const
		{
			if (c == RightSquareBracket)
			{
				return Xml::State::CDataEnd1;
			}
			return state;
		}

		Xml::State CDataEnd1(char c) const // NOLINT
		{
			if (c == RightSquareBracket)
			{
				return Xml::State::CDataEnd2;
			}
			return Xml::State::CDataValue;
		}

		Xml::State CDataEnd2(char c) const // NOLINT
		{
			if (c == RightAngleBracket)
			{
				return Xml::State::Start;
			}
			return Xml::State::CDataValue;
		}

		Xml::State TextEntity(char c) const
		{
			if (c == SemiColon)
			{
				return State::Text;
			}
			// todo: validate chars, more states for decimal, hex numbers
			return state;
		}

		Xml::State AttributeEntity(char c) const
		{
			if (c == SemiColon)
			{
				return State::AttributeValue;
			}
			// todo: validate chars, more states for decimal, hex numbers
			return state;
		}

		// state functions
		//////////////////////

		// must be enum order
		static constexpr std::array<StateFunction, static_cast<int>(State::Count)> stateFunctions =
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
			&StateEngine::AttributeSpace,
			&StateEngine::AttributeName,
			&StateEngine::AttributeNameSpace,
			&StateEngine::AttributeValueStart,
			&StateEngine::AttributeValue,
			&StateEngine::AttributeEnd,
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

			&StateEngine::TextEntity,
			&StateEngine::AttributeEntity,
		};
	};
}