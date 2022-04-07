#pragma once

#include <GLib/Xml/Utils.h>

#include <iomanip>
#include <sstream>
#include <stack>

namespace GLib::Xml
{
	class Printer
	{
		static constexpr int textDepthNotSet = -1;

		const bool format;
		bool elementOpen {};
		bool isFirstElement {true};
		int depth {};
		int textDepth;
		std::ostringstream s;
		std::stack<std::string> stack;

	public:
		explicit Printer(bool format = true)
			: format(format)
			, textDepth(textDepthNotSet)
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
			if (textDepth == textDepthNotSet && !isFirstElement && elementFormat)
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

		void PushAttribute(std::string_view name, std::string_view value)
		{
			AssertTrue(elementOpen, "Element not open");
			s << ' ' << name << R"(=")";
			Text(value);
			s << '"';
		}

		void PushAttribute(std::string_view name, int64_t value)
		{
			PushAttribute(name, std::to_string(value).c_str());
		}

		void PushText(std::string_view text)
		{
			textDepth = depth - 1;
			CloseJustOpenedElement();
			Text(text);
		}

		void PushDocType(std::string_view docType)
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
				if (textDepth == textDepthNotSet && elementFormat)
				{
					s << std::endl << std::setw(depth) << "";
				}
				s << "</" << name << '>';
			}

			if (textDepth == depth)
			{
				textDepth = textDepthNotSet;
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

		[[nodiscard]] std::string Xml() const
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

		void Text(std::string_view value)
		{
			Utils::Escape(value, s);
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