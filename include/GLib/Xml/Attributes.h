#pragma once

#include <GLib/Xml/AttributeIterator.h>
#include <GLib/Xml/NameSpaceManager.h>

namespace GLib::Xml
{
	class Attributes
	{
		std::string_view data;
		const NameSpaceManager * manager;

	public:
		Attributes(std::string_view data = {}, const NameSpaceManager * manager = {})
			: data(data)
			, manager(manager)
		{}

		AttributeIterator begin() const
		{
			return {manager, data.data(), data.data() + data.size()};
		}

		static AttributeIterator end()
		{
			return {};
		}

		bool empty() const
		{
			return data.empty();
		}

		const std::string_view & Value() const
		{
			return data;
		}
	};
}