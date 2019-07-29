#pragma once

#include <string_view>

namespace GLib::Xml::Utils
{
	using PtrPair = std::pair<const char *, const char *>;

	inline std::string_view ToStringView(const PtrPair & value)
	{
		return {value.first, static_cast<size_t>(value.second - value.first) };
	}

	inline std::string_view ToStringView(const char * start, const char * end)
	{
		return {start, static_cast<size_t>(end - start) };
	}
}