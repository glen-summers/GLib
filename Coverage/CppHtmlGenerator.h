#pragma once

#include "GLib/Cpp/Iterator.h"
#include "GLib/Xml/Utils.h"
#include "GLib/split.h"

#include <vector>
#include <unordered_map>

// dumb version, search for contiguous chunks
inline void VisibleWhitespace(std::string_view value, std::ostream & s)
{
	for (size_t startPos = 0;;)
	{
		size_t const find = value.find_first_of(" 	", startPos);
		if (find == std::string::npos)
		{
			s << value.substr(startPos);
			break;
		}

		std::string_view replacement = value[find]==' ' ? "\xC2\xB7" : " \xE2\x86\x92 ";
		s << value.substr(startPos, find - startPos) << replacement;
		startPos = find + 1;
	}
}

inline void Htmlify(const GLib::Cpp::Holder & code, std::ostream & out)
{
	static std::unordered_map<GLib::Cpp::State, char> styles =
	{
		{ GLib::Cpp::State::WhiteSpace, 'w'},
		{ GLib::Cpp::State::CommentLine, 'c'},
		{ GLib::Cpp::State::CommentBlock, 'c'},
		{ GLib::Cpp::State::String, 's'},
		{ GLib::Cpp::State::RawString, 's'},
		{ GLib::Cpp::State::Directive, 'd'},
	};

	for (auto f : code)
	{
		auto it = styles.find(f.first);
		{
			if (it == styles.end())
			{
				GLib::Xml::Utils::Escape(f.second, out);
				continue;
			}

			auto splitter = GLib::Util::SplitterView(f.second, "\n");
			for (auto sit = splitter.begin(), end = splitter.end(); sit != end;)
			{
				GLib::Cpp::State state = it->first;
				std::string_view value = *sit;
				if (value.find('\n') != std::string_view::npos)
				{
					throw std::logic_error("newline"); // remove
				}

				if (!value.empty())
				{
					out << "<span class=\"" << it->second << "\">";
				}

				if (state == GLib::Cpp::State::WhiteSpace) // +others? all? wsp calls escape?
				{
					VisibleWhitespace(*sit, out);
				}
				else
				{
					GLib::Xml::Utils::Escape(*sit, out);
				}

				if (!value.empty())
				{
					out << "</span>";
				}

				if (++sit == end)
				{
					break;
				}

				out << std::endl;
			}
		}
	}
}