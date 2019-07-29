#pragma once

#include "GLib/Xml/Attributes.h"

namespace GLib::Xml
{
	enum class ElementType : int
	{
		Open, Empty, Close
	};

	struct Element // class
	{
		std::string_view qName;
		std::string_view name;
		std::string_view nameSpace;
		std::string_view outerXml;
		ElementType type {};
		Attributes attributes;
		size_t depth {}; // move/remove?

		Element(std::string_view qName, std::string_view name, std::string_view nameSpace, ElementType type, Attributes attributes={})
			: qName(qName), name(name), nameSpace(nameSpace), type(type), attributes(attributes)
		{}

		Element(std::string_view name, ElementType type, Attributes attributes={})
			: qName(name), name(name), type(type), attributes(attributes)
		{}

		Element() = default;
	};
}