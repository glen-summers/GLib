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

		bool const format;
		bool elementOpen {};
		bool isFirstElement {true};
		int depth {};
		int textDepth;
		std::ostringstream stm;
		std::stack<std::string> stack;

	public:
		explicit Printer(bool const format = true)
			: format(format)
			, textDepth(textDepthNotSet)
		{}

		void PushDeclaration()
		{
			stm << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << std::endl;
		}

		void OpenElement(std::string const & name)
		{
			OpenElement(name, format);
		}

		void OpenElement(std::string const & name, bool const elementFormat)
		{
			CloseJustOpenedElement();
			stack.push(name);
			if (textDepth == textDepthNotSet && !isFirstElement && elementFormat)
			{
				stm << std::endl;
			}
			if (elementFormat)
			{
				stm << std::string(depth, ' ');
			}
			stm << '<' << name;
			elementOpen = true;
			isFirstElement = false;
			++depth;
		}

		void PushAttribute(std::string_view const name, std::string_view const value)
		{
			AssertTrue(elementOpen, "Element not open");
			stm << ' ' << name << R"(=")";
			Text(value);
			stm << '"';
		}

		void PushAttribute(std::string_view const name, const int64_t value)
		{
			PushAttribute(name, std::to_string(value).c_str());
		}

		void PushText(std::string_view const text)
		{
			textDepth = depth - 1;
			CloseJustOpenedElement();
			Text(text);
		}

		void PushDocType(std::string_view const docType)
		{
			stm << "<!DOCTYPE " << docType << '>' << std::endl;
		}

		void CloseElement()
		{
			CloseElement(format);
		}

		void CloseElement(bool const elementFormat)
		{
			--depth;
			auto const name = stack.top();
			stack.pop();
			if (elementOpen)
			{
				stm << "/>";
			}
			else
			{
				if (textDepth == textDepthNotSet && elementFormat)
				{
					stm << std::endl << std::setw(depth) << "";
				}
				stm << "</" << name << '>';
			}

			if (textDepth == depth)
			{
				textDepth = textDepthNotSet;
			}

			if (depth == 0 && elementFormat)
			{
				stm << std::endl;
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
			return stm.str();
		}

	private:
		void CloseJustOpenedElement()
		{
			if (elementOpen)
			{
				stm << '>';
				elementOpen = false;
			}
		}

		void Text(std::string_view const value)
		{
			Utils::Escape(value, stm);
		}

		static void AssertTrue(bool const value, char const * message)
		{
			if (!value)
			{
				throw std::runtime_error(message);
			}
		}
	};
}