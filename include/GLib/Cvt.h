#pragma once

#include <codecvt>
#include <locale>
#include <string>

namespace GLib::Cvt
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // until have proper\efficient fix
#endif
	namespace Detail
	{
#ifdef __linux__
		using Converter = std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>;
#elif _WIN32
		using Converter = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>;
#endif
	}

	inline std::wstring A2W(std::string_view const value)
	{
		return Detail::Converter().from_bytes(value.data(), value.data() + value.size());
		// Detail::AssertTrue(convert.converted() == s.size(), "conversion failed");
	}

	inline std::string W2A(std::wstring_view const value)
	{
		return Detail::Converter().to_bytes(value.data(), value.data() + value.size());
		// Detail::AssertTrue(convert.converted() == s.size(), "conversion failed");
	}

	template <typename T>
	std::string P2A(T const & value)
	{
		return W2A(value.wstring());
	}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
}