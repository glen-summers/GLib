#pragma once

#include "GLib/XmlPrinter.h"

#include <filesystem>

// strict XHTML?
class HtmlPrinter : public XmlPrinter
{
public:
	HtmlPrinter(const std::string & title, const std::filesystem::path & css = {})
		: XmlPrinter(true)
	{
		PushDocType("html");
		OpenElement("html");
		OpenElement("head");
		OpenElement("meta");
		PushAttribute("charset", "UTF-8");
		OpenElement("title");
		PushText(title);
		CloseElement(); // title
		CloseElement(); // meta
		CloseElement(); // head

		if (!css.empty())
		{
			OpenElement("link");
			PushAttribute("rel", "stylesheet");
			PushAttribute("type", "text/css");
			PushAttribute("href", css.generic_u8string());
			CloseElement(); // link
		}
		CloseElement(); // head
		OpenElement("body");
	}

	void Anchor(const std::filesystem::path & path, const std::string & text)
	{
		OpenElement("a", false);
		PushAttribute("href", path.generic_u8string());
		PushText(text);
		CloseElement(false);
	}

	void Span(const std::string & text, const std::string & cls)
	{
		OpenElement("span", false);
		PushAttribute("class", cls);
		PushText(text);
		CloseElement(false);
	}

	void OpenTable() // parms
	{
		OpenElement("table");
		PushAttribute("cellpadding", "0");
		PushAttribute("cellspacing", "0");
		PushAttribute("border", "0");
	}

	void LineBreak(const char * element = "hr")
	{
		OpenElement(element);
		CloseElement(element);
	}
};
