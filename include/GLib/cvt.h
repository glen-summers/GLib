#pragma once

#include <string>
#include <codecvt>
#include <locale>

namespace GLib
{
	namespace Cvt
	{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)	// until have proper\efficient fix
#endif
		inline std::wstring a2w(const char * s)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
			return convert.from_bytes(s);
		}

		inline std::wstring a2w(const std::string & s)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
			return convert.from_bytes(s);
		}

		inline std::string w2a(const wchar_t * s)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
			return convert.to_bytes(s);
		}

		inline std::string w2a(const wchar_t * s, const wchar_t * e)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
			return convert.to_bytes(s, e);
		}

		inline std::string w2a(const std::wstring & s)
		{
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
			return convert.to_bytes(s);
		}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	}
}