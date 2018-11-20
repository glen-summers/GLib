#pragma once

#include <string>
#include <codecvt>
#include <locale>

#ifdef __linux__
#include <unicode/ucol.h>
#elif _MSC_VER
#include <icu.h>
#pragma comment (lib, "icuuc.lib")
#pragma comment (lib, "icuin.lib")
#endif

namespace GLib
{
	// rename file and namespace, to StringUtils or split none conversion methods out
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
			inline void AssertNoError(UErrorCode error, const char * msg)
			{
				if (U_FAILURE(error))
				{
					throw std::runtime_error(std::string(msg) + " : " + ::u_errorName(error));
				}
				// warn error != U_ZERO_ERROR
			}

			inline void AssertTrue(bool value, const char * msg)
			{
				if (!value)
				{
					throw std::runtime_error(std::string(msg));
				}
			}

			struct CollatorCloser
			{
				void operator()(UCollator *collator) const noexcept
				{
					::ucol_close(collator);
				}
			};

			typedef std::unique_ptr<UCollator, CollatorCloser> CollatorPtr;

			inline CollatorPtr MakeCollator(const char * locale = nullptr)
			{
				UErrorCode error = U_ZERO_ERROR;
				UCollator * col = ::ucol_open(locale, &error);
				AssertNoError(error, "ucol_open");

				// throw if locale is specified but is not used
				AssertTrue(locale == nullptr || error != U_USING_DEFAULT_WARNING, "ucol_open");
				return CollatorPtr(col);
			}
		}

		/* ICU 8->16, but no 8->32
		inline std::u16string a2w16(const char * str)
		{
			// ifdef ICU...
			int32_t destLength;
			UErrorCode error{};
			//int32_t length = Util::CheckedCast<int32_t>(::strlen(str));
			int32_t length = static_cast<int32_t>(::strlen(str));
			::u_strFromUTF8(nullptr, 0, &destLength, str, length, &error);
			Detail::AssertNoError(error, "u_strFromUTF8");
			Util::StackOrHeap<UChar, 256> s;
			s.EnsureSize(destLength);
			::u_strFromUTF8(s.Get(), static_cast<int32_t>(s.size()), &destLength, str, length, &error);
			Detail::AssertNoError(error, "u_strFromUTF8");
			return s.Get();
			//#else...
			//return Detail::Converter().from_bytes(s);
		}*/

		inline std::wstring a2w(const std::string & s)
		{
			return Detail::Converter().from_bytes(s);
			// if(cnv.converted() < s.size()) throw std::runtime_error("conversion failed");
		}

		inline std::string w2a(const wchar_t * s)
		{
			return Detail::Converter().to_bytes(s);
		}

		inline std::string w2a(const wchar_t * s, const wchar_t * e)
		{
			return Detail::Converter().to_bytes(s, e);
		}

		inline std::string w2a(const std::wstring & s)
		{
			return Detail::Converter().to_bytes(s);
		}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

		enum class CompareResult : int { Less, Equal , Greater} ;

		inline CompareResult CompareNoCase(const char * s1, size_t s1size, const char * s2, size_t s2size, const char * locale = nullptr)
		{
			auto collator = Detail::MakeCollator(locale); // cache? perf test
			::ucol_setStrength(collator.get(), UCOL_SECONDARY);

			UCharIterator i1, i2;
			::uiter_setUTF8(&i1, &s1[0], static_cast<int32_t>(s1size));
			::uiter_setUTF8(&i2, &s2[0], static_cast<int32_t>(s2size));

			UErrorCode error = U_ZERO_ERROR;
			UCollationResult result = ::ucol_strcollIter(collator.get(), &i1, &i2, &error);
			Detail::AssertNoError(error, "ucol_strcollIter");
			return CompareResult { static_cast<int>(result) };
		}

		inline CompareResult CompareNoCase(const std::string & s1, const std::string & s2, const char * locale = nullptr)
		{
			return CompareNoCase(s1.c_str(), s1.size(), s2.c_str(), s2.size(), locale);
		}
	}
}
