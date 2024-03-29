#pragma once

#include <array>
#include <ostream>
#include <string_view>
#include <utility>

namespace GLib::Xml::Utils
{
	using PtrPair = std::pair<std::string_view::const_iterator, std::string_view::const_iterator>;

	inline std::string_view ToStringView(PtrPair const & value)
	{
		return {&value.first[0], static_cast<size_t>(value.second - value.first)};
	}

	using Entity = std::pair<std::string_view, char>;

	static constexpr Entity Quot {"&quot;", '\"'};
	static constexpr Entity Amp {"&amp;", '&'};
	static constexpr Entity Apos {"&apos;", '\''};
	static constexpr Entity Open {"&lt;", '<'};
	static constexpr Entity Close {"&gt;", '>'};

	static constexpr auto EntitySize = 5;
	static constexpr std::array Entities {Quot, Amp, Apos, Open, Close};

	inline std::ostream & Escape(std::string_view value, std::ostream & out)
	{
		for (size_t startPos = 0;;)
		{
			std::string_view replacement;
			size_t pos = std::string::npos;
			for (auto const & [escaped, unescaped] : Entities)
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