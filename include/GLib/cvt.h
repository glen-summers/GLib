#pragma once

#include <string>
#include <codecvt>

namespace GLib
{
	namespace Cvt
	{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)	// until have proper\efficient fix
#endif
		namespace Detail
		{
#ifdef __linux__
			typedef std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> Converter;
#elif _WIN32
			typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> Converter;
#endif
		}

		inline std::wstring a2w(const std::string & s)
		{
			return Detail::Converter().from_bytes(s);
			//Detail::AssertTrue(convert.converted() == s.size(), "conversion failed");
		}

		inline std::string w2a(const std::wstring & s)
		{
			return Detail::Converter().to_bytes(s);
			//Detail::AssertTrue(convert.converted() == s.size(), "conversion failed");
		}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
	}
}
