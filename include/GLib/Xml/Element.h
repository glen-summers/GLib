#pragma once

#include "GLib/Xml/Attributes.h"

namespace GLib::Xml
{
	enum class ElementType : int
	{
		Open, Empty, Close, Text, Comment
	};

	class Element
	{
		friend class Iterator; // avoid

		std::string_view qName;
		std::string_view name;
		std::string_view nameSpace;
		std::string_view outerXml;
		ElementType type {};
		GLib::Xml::Attributes attributes;
		size_t depth {}; // move/remove?
		std::string_view text; // value?

	public:
		Element(std::string_view qName, std::string_view name, std::string_view nameSpace, ElementType type, Attributes attributes={})
			: qName(qName), name(name), nameSpace(nameSpace), type(type), attributes(attributes)
		{}

		Element(std::string_view name, ElementType type, Attributes attributes={})
			: qName(name), name(name), type(type), attributes(attributes)
		{}

		Element(ElementType type, std::string_view text)
			: type(type), text(text)
		{}

		Element() = default;

		const std::string_view & QName() const
		{
			return qName;
		}

		const std::string_view & Name() const
		{
			return name;
		}

		const std::string_view & NameSpace() const
		{
			return nameSpace;
		}

		const std::string_view & OuterXml() const
		{
			return outerXml;
		}

		ElementType Type() const
		{
			return type;
		}

		const GLib::Xml::Attributes & Attributes() const
		{
			return attributes;
		}

		size_t Depth() const  // move/remove?
		{
			return depth;
		}

		// haxor
		const std::string_view & Text() const
		{
			return text;
		}
	};
}