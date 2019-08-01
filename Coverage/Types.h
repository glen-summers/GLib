#pragma once

#include <GLib/NoCase.h>

#include <string>
#include <map>
#include <set>

template<> struct GLib::NoCaseLess<wchar_t>
{
	bool operator()(const std::wstring & s1, const std::wstring &s2) const
	{
		return _wcsicmp(s1.c_str(), s2.c_str()) < 0;
	}
};

template <typename T, typename Value>
using CaseInsensitiveMap = std::map<std::basic_string<T>, Value, GLib::NoCaseLess<T>>;

template <typename T>
using CaseInsensitiveSet = std::set<std::basic_string<T>, GLib::NoCaseLess<T>>;

using WideStrings = CaseInsensitiveSet<wchar_t>;
using Strings = CaseInsensitiveSet<char>;

using Lines = std::map<unsigned int, bool>;
using FileLines = CaseInsensitiveMap<wchar_t, Lines>;
class Address;
using Addresses = std::map<uint64_t, Address>;
