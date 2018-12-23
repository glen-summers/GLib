#pragma once

#include <sstream>
#include <stack>

class XmlPrinter
{
	static constexpr int TextDepthNotSet = -1;

	bool elementOpen = false;
	bool isFirstElement = true;
	int depth = 0;
	int textDepth = -1;
	std::ostringstream s;
	std::stack<std::string> stack;
	
	struct Entity
	{
		const char * escaped;
		char c;
	};

	inline static const Entity entities[]
	{
		{ "&quot;", '\"' },
		{ "&amp;" , '&'  },
		{ "&apos;", '\'' },
		{ "&lt;"  , '<'  },
		{ "&gt;"  , '>'  }
	};

	void Text(std::string && value)
	{
		for (const auto & e : entities)
		{
			ReplaceAll(value, e.c, e.escaped);
		}
		s << value;
	}

	static void ReplaceAll(std::string & value, char c, const char * replacement)
	{
		for (size_t start_pos = value.find(c); start_pos != std::string::npos; start_pos = value.find(c, start_pos + ::strlen(replacement)))
		{
			value.replace(start_pos, 1, replacement);
		}
	}

public:
	void PushDeclaration()
	{
		s << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << std::endl;
	}

	void OpenElement(const char * name)
	{
		CloseJustOpenedElement();
		stack.push(name);
		if (textDepth == TextDepthNotSet && !isFirstElement)
		{
			s << std::endl;
		}
		s << std::string(depth, ' ') << '<' << name;
		elementOpen = true;
		isFirstElement = false;
		++depth;
	}

	void PushAttribute(const char * name, const char * value)
	{
		AssertTrue(elementOpen, "Element not open");
		s << ' ' << name << R"(=")";
		Text(value);
		s << '"';
	}

	void PushAttribute(const char * name, int64_t value)
	{
		PushAttribute(name, std::to_string(value).c_str());
	}

	void PushText(const char * text)
	{
		textDepth = depth - 1;
		CloseJustOpenedElement();
		Text(text);
	}

	void CloseElement()
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
			if (textDepth == TextDepthNotSet)
			{
				s
					<< std::endl
					<< std::string(depth, ' ');
			}
			s << "</" << name << '>';
		}

		if (textDepth == depth)
		{
			textDepth = TextDepthNotSet;
		}

		if (depth == 0)
		{
			s << std::endl;
		}

		elementOpen = false;
	}

	std::string Xml() const
	{
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

	static void AssertTrue(bool value, const char * message)
	{
		if (!value)
		{
			throw std::runtime_error(message);
		}
	}
};

