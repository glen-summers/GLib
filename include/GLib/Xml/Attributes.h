#pragma once

#include "GLib/Xml/NameSpaceManager.h"
#include "GLib/Xml/AttributeIterator.h"

namespace GLib::Xml
{
	class Attributes
	{
		const NameSpaceManager * manager;
		std::string_view data;

	public:
		Attributes(std::string_view data={}, const NameSpaceManager * manager=nullptr)
			: data(data)
			, manager(manager)
		{}

		AttributeIterator begin() const
		{
			return {manager, data.data(), data.data() + data.size()};
		}

		AttributeIterator end() const
		{
			return {};
		}

		bool empty() const
		{
			return data.empty();
		}
	};
}