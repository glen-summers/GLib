#pragma once

#include <sstream>
#include <stack>

// move to xml dir
class XmlPrinter
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
	XmlPrinter(bool format = true)
		: format(format)
		, textDepth(TextDepthNotSet)
	{}

	void PushDeclaration()
	{
		s << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << std::endl;
	}

	void OpenElement(const char * name)
	{
		OpenElement(name, format);
	}

	void OpenElement(const char * name, bool elementFormat)
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

	void PushAttribute(const char * name, const char * value)
	{
		AssertTrue(elementOpen, "Element not open");
		s << ' ' << name << R"(=")";
		Text(value);
		s << '"';
	}

	void PushAttribute(const char * name, const std::string & value)
	{
		AssertTrue(elementOpen, "Element not open");
		s << ' ' << name << R"(=")";
		Text(value.c_str());
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

	void PushText(const std::string & text)
	{
		PushText(text.c_str());
	}

	void PushDocType(const char * docType)
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

	void CloseJustOpenedElement()
	{
		if (elementOpen)
		{
			s << '>';
			elementOpen = false;
		}
	}

	void Text(std::string && value)
	{
		Escape(value);
		s << value;
	}

	static void Escape(std::string & value)
	{
		// better to stream to new string, if no change move?
		for (size_t startPos = 0;;)
		{
			const char * replacement = {};
			size_t pos = std::string::npos;
			for (const auto & e : entities)
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
				return;
			}
			value.replace(pos, 1, replacement);
			startPos = pos + ::strlen(replacement);
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

