#pragma once

#include <array>
#include <cctype>

namespace GLib::Xml
{
	using EnumType = uint8_t;

	enum class State : EnumType
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
		static constexpr char leftAngleBracket = '<';
		static constexpr char rightAngleBracket = '>';
		static constexpr char forwardSlash = '/';
		static constexpr char equals = '=';
		static constexpr char doubleQuote = '"';
		static constexpr char singleQuote = '\'';
		static constexpr char colon = ':';
		static constexpr char semiColon = ';';
		static constexpr char underscore = '_';
		static constexpr char fullStop = '.';
		static constexpr char hyphen = '-';
		static constexpr char exclamation = '!';
		static constexpr char dash = '-';
		static constexpr char questionMark = '?';
		static constexpr char leftSquareBracket = '[';
		static constexpr char rightSquareBracket = ']';
		static constexpr char ampersand = '&';

		static constexpr EnumType continuationMask = 0x80U;

		using StateFunction = State (StateEngine::*)(char) const;

		// use Phase : Prologue, Document, End
		// could also manage depth here to determine end
		// but try in iterator first?
		State state;
		bool mutable isProlog {true};
		bool mutable hasDocTypeDecl {};
		bool mutable hasContent {};
		char mutable attributeQuoteChar {};
		StateFunction stateFunction;

	public:
		explicit StateEngine(State state = State::Start)
			: state(state)
			, stateFunction(stateFunctions.at(static_cast<EnumType>(state)))
		{}

		State GetState() const
		{
			return state;
		}

		bool HasRootElement() const
		{
			return !isProlog;
		}

		State Push(char const value)
		{
			SetState((this->*stateFunction)(value));
			return state;
		}

	private:
		static bool IsContinuation(char const c)
		{
			return (static_cast<EnumType>(c) & continuationMask) != 0;
		}

		static bool IsWhiteSpace(char const c)
		{
			return !IsContinuation(c) && std::isspace(c) != 0;
		}

		static bool IsNameStart(char const c)
		{
			return IsContinuation(c) || std::isalpha(c) != 0 || c == colon || c == underscore; // check docs
		}

		static bool IsName(char const c)
		{
			return IsNameStart(c) || std::isdigit(c) != 0 || c == fullStop || c == hyphen; // check docs
		}

		static bool IsAllowedTextCharacter(char const c)
		{
			return c != leftAngleBracket;
		}

		void SetState(State const newState)
		{
			if (newState != state)
			{
				state = newState;
				stateFunction = stateFunctions.at(static_cast<EnumType>(state));
			}
		}

		////////////////////////
		// state functions
		State Error(char const c) const
		{
			static_cast<void>(c);
			return state;
		}

		State Start(char const c) const
		{
			if (c == leftAngleBracket)
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

		State DocTypeDecl(char const c) const
		{
			if (IsName(c) || IsWhiteSpace(c))
			{
				return state;
			}
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			return state;
		}

		State ElementStart(char const c) const
		{
			if (IsNameStart(c))
			{
				isProlog = false;
				hasContent = true;
				return State::ElementName;
			}
			if (c == forwardSlash)
			{
				return State::ElementEnd;
			}
			if (c == exclamation)
			{
				hasContent = true;
				return State::Bang;
			}
			if (c == questionMark && !hasContent)
			{
				return State::XmlDeclaration;
			}
			return State::Error;
		}

		State ElementEnd(char const c) const
		{
			static_cast<void>(this);
			if (IsNameStart(c))
			{
				return State::ElementEndName;
			}
			return State::Error;
		}

		State ElementEndName(char const c) const
		{
			if (IsName(c))
			{
				return state;
			}
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			if (IsWhiteSpace(c))
			{
				return State::ElementEndSpace;
			}
			return State::Error;
		}

		State ElementEndSpace(char const c) const
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		State ElementName(char const c) const
		{
			if (IsName(c))
			{
				return state;
			}
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			if (c == forwardSlash)
			{
				return State::EmptyElement;
			}
			if (IsWhiteSpace(c))
			{
				return State::AttributeSpace;
			}
			return State::Error;
		}

		State EmptyElement(char const c) const
		{
			static_cast<void>(this);
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		State AttributeSpace(char const c) const
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			if (c == forwardSlash)
			{
				return State::EmptyElement;
			}
			if (IsNameStart(c))
			{
				return State::AttributeName;
			}
			return State::Error;
		}

		State AttributeName(char const c) const
		{
			if (IsName(c))
			{
				return state;
			}
			if (IsWhiteSpace(c))
			{
				return State::AttributeNameSpace;
			}
			if (c == equals)
			{
				return State::AttributeValueStart;
			}
			return State::Error;
		}

		State AttributeNameSpace(char const c) const
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == equals)
			{
				return State::AttributeValueStart;
			}
			return State::Error;
		}

		State AttributeValueStart(char const c) const
		{
			if (IsWhiteSpace(c))
			{
				return state;
			}
			if (c == doubleQuote || c == singleQuote)
			{
				attributeQuoteChar = c;
				return State::AttributeValue;
			}
			return State::Error;
		}

		State AttributeValue(char const c) const
		{
			if (c == attributeQuoteChar)
			{
				return State::AttributeEnd;
			}
			if (c == ampersand)
			{
				return State::AttributeEntity;
			}
			if (IsAllowedTextCharacter(c))
			{
				return state;
			}
			return State::Error;
		}

		State AttributeEnd(char const c) const
		{
			static_cast<void>(this);
			if (IsWhiteSpace(c))
			{
				return State::AttributeSpace;
			}
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			if (c == forwardSlash)
			{
				return State::EmptyElement;
			}
			return State::Error;
		}

		State Text(char const c) const
		{
			if (c == leftAngleBracket)
			{
				return State::ElementStart;
			}
			if (c == ampersand)
			{
				return State::TextEntity;
			}
			if (IsAllowedTextCharacter(c))
			{
				return state;
			}
			return State::Error;
		}

		State Bang(char const c) const
		{
			if (c == dash)
			{
				return State::CommentStartDash;
			}
			if (c == leftSquareBracket)
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

		State CommentStartDash(char const c) const
		{
			static_cast<void>(this);
			if (c == dash)
			{
				return State::Comment;
			}
			return State::Error;
		}

		State Comment(char const c) const
		{
			if (c == dash)
			{
				return State::CommentEndDash;
			}
			return state;
		}

		State CommentEndDash(char const c) const
		{
			static_cast<void>(this);
			if (c == dash)
			{
				return State::CommentEnd;
			}
			return State::Comment;
		}

		State CommentEnd(char const c) const
		{
			static_cast<void>(this);
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		State XmlDeclaration(char const c) const
		{
			if (c == questionMark)
			{
				return State::EmptyElement;
			}
			return state;
		}

		State CDataName(char const c) const
		{
			if (c == leftSquareBracket)
			{
				return State::CDataValue;
			}
			return state;
		}

		State CDataValue(char const c) const
		{
			if (c == rightSquareBracket)
			{
				return State::CDataEnd1;
			}
			return state;
		}

		State CDataEnd1(char const c) const
		{
			static_cast<void>(this);
			if (c == rightSquareBracket)
			{
				return State::CDataEnd2;
			}
			return State::CDataValue;
		}

		State CDataEnd2(char const c) const
		{
			static_cast<void>(this);
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			return State::CDataValue;
		}

		State TextEntity(char const c) const
		{
			if (c == semiColon)
			{
				return State::Text;
			}
			// todo: validate chars, more states for decimal, hex numbers
			return state;
		}

		State AttributeEntity(char const c) const
		{
			if (c == semiColon)
			{
				return State::AttributeValue;
			}
			// todo: validate chars, more states for decimal, hex numbers
			return state;
		}

		// state functions
		//////////////////////

		// must be enum order
		static constexpr std::array<StateFunction, static_cast<int>(State::Count)> stateFunctions = {
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