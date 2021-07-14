#pragma once

#include <array>
#include <ostream>
#include <string_view>
#include <utility>

namespace GLib::Xml::Utils
{
	using PtrPair = std::pair<std::string_view::const_iterator, std::string_view::const_iterator>;

	inline std::string_view ToStringView(const PtrPair & value)
	{
		return {&value.first[0], static_cast<size_t>(value.second - value.first)};
	}

	using Entity = std::pair<std::string_view, char>;

	// clang-format off
	static constexpr auto EntitySize = 5;
	static constexpr std::array<Entity, EntitySize> entities
	{
		Entity{ "&quot;", '\"' },
		Entity{ "&amp;" , '&'  },
		Entity{ "&apos;", '\'' },
		Entity{ "&lt;"  , '<'  },
		Entity{ "&gt;"  , '>'  }
	};
	// clang-format on

	inline std::ostream & Escape(std::string_view value, std::ostream & out)
	{
		for (size_t startPos = 0;;)
		{
			std::string_view replacement;
			size_t pos = std::string::npos;
			for (const auto & [escaped, unescaped] : entities)
			{
				size_t const find = value.find(unescaped, startPos);
				if (find != std::string::npos && find < pos)
				{
					pos = find;
					replacement = escaped;
				}
			}
			if (pos == std::string::npos)
			{
				out << value.substr(startPos, value.size() - startPos) << replacement;
				break;
			}
			out << value.substr(startPos, pos - startPos) << replacement;
			startPos = pos + 1;
		}
		return out;
	}
}