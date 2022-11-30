#pragma once

#include <GLib/Xml/Attributes.h>

namespace GLib::Xml
{
	enum class ElementType : uint8_t
	{
		Open,
		Empty,
		Close,
		Text,
		Comment
	};

	class Element
	{
		friend class Iterator; // avoid

		std::string_view qName;
		std::string_view name;
		std::string_view nameSpace;
		std::string_view outerXml;
		ElementType type {};
		Attributes attributes;
		size_t depth {};			 // move/remove?
		std::string_view text; // value?

	public:
		Element(std::string_view const qName, std::string_view const name, std::string_view const nameSpace, ElementType const type,
						Attributes const attributes = {})
			: qName(qName)
			, name(name)
			, nameSpace(nameSpace)
			, type(type)
			, attributes(attributes)
		{}

		Element(std::string_view const name, ElementType const type, Attributes const attributes = {})
			: qName(name)
			, name(name)
			, type(type)
			, attributes(attributes)
		{}

		Element(ElementType const type, std::string_view const text)
			: type(type)
			, text(text)
		{}

		Element() = default;

		[[nodiscard]] std::string_view QName() const
		{
			return qName;
		}

		[[nodiscard]] std::string_view Name() const
		{
			return name;
		}

		[[nodiscard]] std::string_view NameSpace() const
		{
			return nameSpace;
		}

		[[nodiscard]] std::string_view OuterXml() const
		{
			return outerXml;
		}

		[[nodiscard]] ElementType Type() const
		{
			return type;
		}

		[[nodiscard]] Attributes const & GetAttributes() const
		{
			return attributes;
		}

		[[nodiscard]] size_t Depth() const // move/remove?
		{
			return depth;
		}

		[[nodiscard]] std::string_view Text() const
		{
			return text;
		}
	};
}