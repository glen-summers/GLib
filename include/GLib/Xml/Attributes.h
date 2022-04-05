#pragma once

#include <GLib/Xml/AttributeIterator.h>
#include <GLib/Xml/NameSpaceManager.h>

namespace GLib::Xml
{
	class Attributes
	{
		std::string_view data;
		const NameSpaceManager * manager = nullptr;

	public:
		Attributes() = default;

		explicit Attributes(std::string_view data)
			: data {data}
		{}

		Attributes(std::string_view data, const NameSpaceManager * manager)
			: data(data)
			, manager(manager)
		{}

		[[nodiscard]] AttributeIterator begin() const
		{
			return {manager, data.begin(), data.end()};
		}

		[[nodiscard]] AttributeIterator end() const
		{
			static_cast<void>(this);
			return {};
		}

		[[nodiscard]] bool Empty() const
		{
			return data.empty();
		}

		[[nodiscard]] std::string_view Value() const
		{
			return data;
		}
	};
}