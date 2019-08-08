#pragma once

#include <array>
#include <sstream>
#include <stack>

// move to xml dir
namespace Xml
{
	namespace Detail
	{
		struct Entity
		{
			const char * escaped;
			char c;
		};

		static constexpr auto EntitySize = 5;
		static constexpr std::array<Entity, EntitySize> entities
		{
			Entity{ "&quot;", '\"' },
			Entity{ "&amp;" , '&'  },
			Entity{ "&apos;", '\'' },
			Entity{ "&lt;"  , '<'  },
			Entity{ "&gt;"  , '>'  }
		};
	}

	inline std::string Escape(std::string && value)
	{
		for (size_t startPos = 0;;)
		{
			const char * replacement = {};
			size_t pos = std::string::npos;
			for (const auto & e : Detail::entities)
			{
				size_t const find = value.find(e.c, startPos);
				if (find != std::string::npos && find < pos)
				{
					pos = find;
					replacement = e.escaped;
				}
			}
			if (pos == std::string::npos)
			{
				break;
			}
			value.replace(pos, 1, replacement);
			startPos = pos + ::strlen(replacement);
		}
		return move(value);
	}

	class Printer
	{
		static constexpr int TextDepthNotSet = -1;

		bool const format;
		bool elementOpen {};
		bool isFirstElement {true};
		int depth {};
		int textDepth;
		std::ostringstream s;
		std::stack<std::string> stack;

	public:
		explicit Printer(bool format = true)
			: format(format)
			, textDepth(TextDepthNotSet)
		{}

		void PushDeclaration()
		{
			s << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << std::endl;
		}

		void OpenElement(const std::string & name)
		{
			OpenElement(name, format);
		}

		void OpenElement(const std::string & name, bool elementFormat)
		{
			CloseJustOpenedElement();
			stack.push(name);
			if (textDepth == TextDepthNotSet && !isFirstElement && elementFormat)
			{
				s << std::endl;
			}
			if (elementFormat)
			{
				s << std::string(depth, ' ');
			}
			s << '<' << name;
			elementOpen = true;
			isFirstElement = false;
			++depth;
		}

		void PushAttribute(const std::string & name, const char * value)
		{
			AssertTrue(elementOpen, "Element not open");
			s << ' ' << name << R"(=")";
			Text(value);
			s << '"';
		}

		void PushAttribute(const std::string & name, const std::string & value)
		{
			AssertTrue(elementOpen, "Element not open");
			s << ' ' << name << R"(=")";
			Text(value);
			s << '"';
		}

		void PushAttribute(const std::string & name, int64_t value)
		{
			PushAttribute(name, std::to_string(value).c_str());
		}

		void PushText(const std::string & text)
		{
			textDepth = depth - 1;
			CloseJustOpenedElement();
			Text(text);
		}

		void PushDocType(const std::string & docType)
		{
			s << "<!DOCTYPE " << docType << '>' << std::endl;
		}

		void CloseElement()
		{
			CloseElement(format);
		}

		void CloseElement(bool elementFormat)
		{
			--depth;
			const auto name = stack.top();
			stack.pop();
			if (elementOpen)
			{
				s << "/>";
			}
			else
			{
				if (textDepth == TextDepthNotSet && elementFormat)
				{
					s << std::endl << std::string(depth, ' ');
				}
				s << "</" << name << '>';
			}

			if (textDepth == depth)
			{
				textDepth = TextDepthNotSet;
			}

			if (depth == 0 && elementFormat)
			{
				s << std::endl;
			}

			elementOpen = false;
		}

		void Close()
		{
			while (!stack.empty())
			{
				CloseElement();
			}
		}

		std::string Xml() const
		{
			if (depth != 0)
			{
				throw std::runtime_error("Element is not closed: " + stack.top());
			}
			return s.str();
		}

	private:
		void CloseJustOpenedElement()
		{
			if (elementOpen)
			{
				s << '>';
				elementOpen = false;
			}
		}

		void Text(const std::string & value)
		{
			s << Escape(std::string{value});
		}

		void Text(std::string && value)
		{
			s << Escape(move(value));
		}

		static void AssertTrue(bool value, const char * message)
		{
			if (!value)
			{
				throw std::runtime_error(message);
			}
		}
	};
}

