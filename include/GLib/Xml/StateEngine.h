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

		static constexpr auto continuationMask = 0x80U;

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
			return (static_cast<unsigned char>(c) & continuationMask) != 0;
		}

		static bool IsWhiteSpace(char c)
		{
			return !IsContinuation(c) && std::isspace(c) != 0;
		}

		static bool IsNameStart(char c)
		{
			return IsContinuation(c) || std::isalpha(c) != 0 || c == colon || c == underscore; // check docs
		}

		static bool IsName(char c)
		{
			return IsNameStart(c) || std::isdigit(c) != 0 || c == fullStop || c == hyphen; // check docs
		}

		static bool IsAllowedTextCharacter(char c)
		{
			return c != leftAngleBracket;
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
			(void) c;
			return state;
		}

		Xml::State Start(char c) const
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

		Xml::State DocTypeDecl(char c) const
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

		Xml::State ElementStart(char c) const
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

		Xml::State ElementEnd(char c) const
		{
			(void) this;
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

		Xml::State ElementEndSpace(char c) const
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

		Xml::State ElementName(char c) const
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

		Xml::State EmptyElement(char c) const
		{
			(void) this;
			if (c == rightAngleBracket)
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
			if (c == equals)
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
			if (c == equals)
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
			if (c == doubleQuote || c == singleQuote)
			{
				attributeQuoteChar = c;
				return State::AttributeValue;
			}
			return State::Error;
		}

		Xml::State AttributeValue(char c) const
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

		Xml::State AttributeEnd(char c) const
		{
			(void) this;
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

		Xml::State Text(char c) const
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

		Xml::State Bang(char c) const
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

		Xml::State CommentStartDash(char c) const
		{
			(void) this;
			if (c == dash)
			{
				return State::Comment;
			}
			return State::Error;
		}

		Xml::State Comment(char c) const
		{
			if (c == dash)
			{
				return State::CommentEndDash;
			}
			return state;
		}

		Xml::State CommentEndDash(char c) const
		{
			(void) this;
			if (c == dash)
			{
				return State::CommentEnd;
			}
			return State::Comment;
		}

		Xml::State CommentEnd(char c) const
		{
			(void) this;
			if (c == rightAngleBracket)
			{
				return State::Start;
			}
			return State::Error;
		}

		Xml::State XmlDeclaration(char c) const
		{
			if (c == questionMark)
			{
				return Xml::State::EmptyElement;
			}
			return state;
		}

		Xml::State CDataName(char c) const
		{
			if (c == leftSquareBracket)
			{
				return Xml::State::CDataValue;
			}
			return state;
		}

		Xml::State CDataValue(char c) const
		{
			if (c == rightSquareBracket)
			{
				return Xml::State::CDataEnd1;
			}
			return state;
		}

		Xml::State CDataEnd1(char c) const
		{
			(void) this;
			if (c == rightSquareBracket)
			{
				return Xml::State::CDataEnd2;
			}
			return Xml::State::CDataValue;
		}

		Xml::State CDataEnd2(char c) const
		{
			(void) this;
			if (c == rightAngleBracket)
			{
				return Xml::State::Start;
			}
			return Xml::State::CDataValue;
		}

		Xml::State TextEntity(char c) const
		{
			if (c == semiColon)
			{
				return State::Text;
			}
			// todo: validate chars, more states for decimal, hex numbers
			return state;
		}

		Xml::State AttributeEntity(char c) const
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