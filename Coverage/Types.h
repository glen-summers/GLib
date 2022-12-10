#pragma once

#include <GLib/NoCase.h>

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <unordered_map>

template <>
struct GLib::NoCaseLess<wchar_t>
{
	bool operator()(std::wstring const & value1, std::wstring const & value2) const
	{
		return _wcsicmp(value1.c_str(), value2.c_str()) < 0;
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
using Addresses = std::unordered_map<uint64_t, Address>;

class FileCoverageData;
using CoverageData = std::map<std::filesystem::path, FileCoverageData>;
