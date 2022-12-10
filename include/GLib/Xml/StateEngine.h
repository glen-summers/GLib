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
		static bool IsContinuation(char const chr)
		{
			return (static_cast<EnumType>(chr) & continuationMask) != 0;
		}

		static bool IsWhiteSpace(char const chr)
		{
			return !IsContinuation(chr) && std::isspace(chr) != 0;
		}

		static bool IsNameStart(char const chr)
		{
			return IsContinuation(chr) || std::isalpha(chr) != 0 || chr == colon || chr == underscore; // check docs
		}

		static bool IsName(char const chr)
		{
			return IsNameStart(chr) || std::isdigit(chr) != 0 || chr == fullStop || chr == hyphen; // check docs
		}

		static bool IsAllowedTextCharacter(char const chr)
		{
			return chr != leftAngleBracket;
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
		State Error(char const chr) const
		{
			static_cast<void>(chr);
			return state;
		}

		State Start(char const chr) const
		{
			if (chr == leftAngleBracket)
			{
				return State::ElementStart;
			}
			if (IsWhiteSpace(chr))
			{
				hasContent = true;
				return state;
			}
			if (!isProlog && IsAllowedTextCharacter(chr))
			{
				return State::Text;
			}
			return State::Error;
		}

		State DocTypeDecl(char const chr) const
		{
			if (IsName(chr) || IsWhiteSpace(chr))
			{
				return state;
			}
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			return state;
		}

		State ElementStart(char const chr) const
		{
			if (IsNameStart(chr))
			{
				isProlog = false;
				hasContent = true;
				return State::ElementName;
			}
			if (chr == forwardSlash)
			{
				return State::ElementEnd;
			}
			if (chr == exclamation)
			{
				hasContent = true;
				return State::Bang;
			}
			if (chr == questionMark && !hasContent)
			{
				return State::XmlDeclaration;
			}
			return State::Error;
		}

		State ElementEnd(char const chr) const
		{
			static_cast<void>(this);
			if (IsNameStart(chr))
			{
				return State::ElementEndName;
			}
			return State::Error;
		}

		State ElementEndName(char const chr) const
		{
			if (IsName(chr))
			{
				return state;
			}
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			if (IsWhiteSpace(chr))
			{
				return State::ElementEndSpace;
			}
			return State::Error;
		}

		State ElementEndSpace(char const chr) const
		{
			if (IsWhiteSpace(chr))
			{
				return state;
			}
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		State ElementName(char const chr) const
		{
			if (IsName(chr))
			{
				return state;
			}
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			if (chr == forwardSlash)
			{
				return State::EmptyElement;
			}
			if (IsWhiteSpace(chr))
			{
				return State::AttributeSpace;
			}
			return State::Error;
		}

		State EmptyElement(char const chr) const
		{
			static_cast<void>(this);
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		State AttributeSpace(char const chr) const
		{
			if (IsWhiteSpace(chr))
			{
				return state;
			}
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			if (chr == forwardSlash)
			{
				return State::EmptyElement;
			}
			if (IsNameStart(chr))
			{
				return State::AttributeName;
			}
			return State::Error;
		}

		State AttributeName(char const chr) const
		{
			if (IsName(chr))
			{
				return state;
			}
			if (IsWhiteSpace(chr))
			{
				return State::AttributeNameSpace;
			}
			if (chr == equals)
			{
				return State::AttributeValueStart;
			}
			return State::Error;
		}

		State AttributeNameSpace(char const chr) const
		{
			if (IsWhiteSpace(chr))
			{
				return state;
			}
			if (chr == equals)
			{
				return State::AttributeValueStart;
			}
			return State::Error;
		}

		State AttributeValueStart(char const chr) const
		{
			if (IsWhiteSpace(chr))
			{
				return state;
			}
			if (chr == doubleQuote || chr == singleQuote)
			{
				attributeQuoteChar = chr;
				return State::AttributeValue;
			}
			return State::Error;
		}

		State AttributeValue(char const chr) const
		{
			if (chr == attributeQuoteChar)
			{
				return State::AttributeEnd;
			}
			if (chr == ampersand)
			{
				return State::AttributeEntity;
			}
			if (IsAllowedTextCharacter(chr))
			{
				return state;
			}
			return State::Error;
		}

		State AttributeEnd(char const chr) const
		{
			static_cast<void>(this);
			if (IsWhiteSpace(chr))
			{
				return State::AttributeSpace;
			}
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			if (chr == forwardSlash)
			{
				return State::EmptyElement;
			}
			return State::Error;
		}

		State Text(char const chr) const
		{
			if (chr == leftAngleBracket)
			{
				return State::ElementStart;
			}
			if (chr == ampersand)
			{
				return State::TextEntity;
			}
			if (IsAllowedTextCharacter(chr))
			{
				return state;
			}
			return State::Error;
		}

		State Bang(char const chr) const
		{
			if (chr == dash)
			{
				return State::CommentStartDash;
			}
			if (chr == leftSquareBracket)
			{
				return State::CDataName;
			}
			if (isProlog && !hasDocTypeDecl && IsNameStart(chr))
			{
				hasDocTypeDecl = true;
				return State::DocTypeDecl;
			}
			return State::Error;
		}

		State CommentStartDash(char const chr) const
		{
			static_cast<void>(this);
			if (chr == dash)
			{
				return State::Comment;
			}
			return State::Error;
		}

		State Comment(char const chr) const
		{
			if (chr == dash)
			{
				return State::CommentEndDash;
			}
			return state;
		}

		State CommentEndDash(char const chr) const
		{
			static_cast<void>(this);
			if (chr == dash)
			{
				return State::CommentEnd;
			}
			return State::Comment;
		}

		State CommentEnd(char const chr) const
		{
			static_cast<void>(this);
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		State XmlDeclaration(char const chr) const
		{
			if (chr == questionMark)
			{
				return State::EmptyElement;
			}
			return state;
		}

		State CDataName(char const chr) const
		{
			if (chr == leftSquareBracket)
			{
				return State::CDataValue;
			}
			return state;
		}

		State CDataValue(char const chr) const
		{
			if (chr == rightSquareBracket)
			{
				return State::CDataEnd1;
			}
			return state;
		}

		State CDataEnd1(char const chr) const
		{
			static_cast<void>(this);
			if (chr == rightSquareBracket)
			{
				return State::CDataEnd2;
			}
			return State::CDataValue;
		}

		State CDataEnd2(char const chr) const
		{
			static_cast<void>(this);
			if (chr == rightAngleBracket)
			{
				return State::Start;
			}
			return State::CDataValue;
		}

		State TextEntity(char const chr) const
		{
			if (chr == semiColon)
			{
				return State::Text;
			}
			// todo: validate chars, more states for decimal, hex numbers
			return state;
		}

		State AttributeEntity(char const chr) const
		{
			if (chr == semiColon)
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