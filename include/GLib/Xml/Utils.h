#pragma once

#include <string_view>

namespace GLib::Xml::Utils
{
	inline std::string_view ToStringView(const char * start, const char * end)
	{
		return {start, static_cast<size_t>(end - start) };
	}
}