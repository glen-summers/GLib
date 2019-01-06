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

typedef CaseInsensitiveSet<wchar_t> WideStrings;
typedef CaseInsensitiveSet<char> Strings;

typedef std::map<unsigned int, bool> Lines;
typedef CaseInsensitiveMap<wchar_t, Lines> FileLines;
class Address;
typedef std::map<uint64_t, Address> Addresses;