#pragma once

#include <GLib/Cpp/Iterator.h>
#include <GLib/Split.h>
#include <GLib/Xml/Utils.h>

#include <cctype>
#include <unordered_map>
#include <unordered_set>

enum class Style : char
{
	WhiteSpace = 'w',
	Comment = 'c',
	String = 's',
	Directive = 'd',
	Keyword = 'k',
	Type = 't'
};

// dumb version, search for contiguous chunks
inline void VisibleWhitespace(std::string_view const value, std::ostream & stm)
{
	for (size_t startPos = 0;;)
	{
		size_t const find = value.find_first_of(" 	", startPos);
		if (find == std::string::npos)
		{
			stm << value.substr(startPos);
			break;
		}

		std::string_view const replacement = value[find] == ' ' ? "\xC2\xB7" : " \xE2\x86\x92 ";
		stm << value.substr(startPos, find - startPos) << replacement;
		startPos = find + 1;
	}
}

inline void OpenSpan(Style const cls, std::ostream & stm)
{
	stm << "<span class=\"" << static_cast<char>(cls) << "\">";
}

inline void CloseSpan(std::ostream & stm)
{
	stm << "</span>";
}

inline void Span(Style const cls, std::string_view const value, std::ostream & stm)
{
	OpenSpan(cls, stm);
	stm << value;
	CloseSpan(stm);
}

inline bool IsKeyword(std::string_view const value)
{
	// clang-format off
	static std::unordered_set<std::string_view> const keywords
	{
		"alignas","alignof","and","and_eq","asm","atomic_cancel","atomic_commit","atomic_noexcept","auto",
		"bitand","bitor","bool","break","case","catch","char","char8_t","char16_t","char32_t","class","compl",
		"concept","const","consteval","constexpr","const_cast","continue","co_await","co_return","co_yield",
		"decltype","default","delete","do","double","dynamic_cast","else","enum","explicit","export","extern",
		"false","float","for","friend","goto","if","inline","int","long","mutable","namespace","new","noexcept",
		"not","not_eq","nullptr","operator","or","or_eq","private","protected","public","reflexpr","register",
		"reinterpret_cast","requires","return","short","signed","sizeof","static","static_assert","static_cast",
		"struct","switch","synchronized","template","this","thread_local","throw","true","try","typedef","typeid",
		"typename","union","unsigned","using","virtual","void","volatile","wchar_t","while","xor","xor_eq",
		"override","final"
	};
	// clang-format on
	return keywords.find(value) != keywords.end();
}

inline bool IsCommonType(std::string_view const value)
{
	// clang-format off
	static std::unordered_set<std::string_view> const types
	{
		"array", "bitset", "deque", "initializer_list", "istringstream", "list", "map", "multimap", "multiset",
		"ostream", "ostringstream", "pair", "queue", "set", "size_t", "string", "string_view", "shared_ptr",
		"stack", "stringstream", "unique_ptr", "unordered_map", "unordered_multimap", "unordered_multiset",
		"unordered_set", "vector"
	};
	// clang-format on
	return types.find(value) != types.end();
}

inline void Htmlify(std::string_view const code, bool const emitWhitespace, std::ostream & out)
{
	// clang-format off
	static std::unordered_map<GLib::Cpp::State, Style> const styles =
	{
		{ GLib::Cpp::State::WhiteSpace, Style::WhiteSpace},
		{ GLib::Cpp::State::CommentLine, Style::Comment},
		{ GLib::Cpp::State::CommentBlock, Style::Comment},
		{ GLib::Cpp::State::String, Style::String},
		{ GLib::Cpp::State::RawString, Style::String},
		{ GLib::Cpp::State::CharacterLiteral, Style::String},
		{ GLib::Cpp::State::Directive, Style::Directive},
	};
	// clang-format on

	auto const alphaNumUnd = [](unsigned char const chr) { return std::isalnum(chr) != 0 || chr == '_'; };
	auto const whitespace = [](unsigned char const chr) { return std::isspace(chr) != 0; };
	auto const escape = [&](std::string_view const value) { GLib::Xml::Utils::Escape(value, out); };
	auto const vis = [&](std::string_view const value) { VisibleWhitespace(value, out); };

	for (auto const & frag : GLib::Cpp::Holder(code, emitWhitespace))
	{
		if (frag.first == GLib::Cpp::State::Code)
		{
			GLib::Util::Split(
				frag.second, alphaNumUnd,
				[&](std::string_view const value)
				{
					if (IsKeyword(value))
					{
						Span(Style::Keyword, value, out);
					}
					else if (IsCommonType(value))
					{
						Span(Style::Type, value, out);
					}
					else
					{
						escape(value);
					}
				},
				escape);

			continue;
		}

		auto const iter = styles.find(frag.first);
		if (iter == styles.end())
		{
			escape(frag.second);
			continue;
		}

		auto const splitter = GLib::Util::SplitterView(frag.second, "\n");
		for (auto sit = splitter.begin(), end = splitter.end(); sit != end;)
		{
			GLib::Cpp::State const state = iter->first;
			std::string_view const value = *sit;

			if (!value.empty())
			{
				OpenSpan(iter->second, out);
				if (state == GLib::Cpp::State::WhiteSpace)
				{
					vis(value);
				}
				else
				{
					GLib::Util::Split(value, whitespace, vis, escape);
				}
				CloseSpan(out);
			}

			if (++sit == end)
			{
				break;
			}

			out << std::endl;
		}
	}
}