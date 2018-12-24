#pragma once

#include <GLib/stackorheap.h>

#include <string>
#include <codecvt>
#include <locale>

#ifdef __linux__
#include <unicode/ucol.h>
#include <unicode/ucasemap.h>
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

			struct UCaseMapCloser
			{
				void operator()(UCaseMap * map) const noexcept
				{
					::ucasemap_close(map);
				}
			};

			typedef std::unique_ptr<UCaseMap, UCaseMapCloser> UCaseMapPtr;

			inline UCaseMapPtr MakeUCaseMap(const char * locale = nullptr)
			{
				UErrorCode error = U_ZERO_ERROR;
				UCaseMap * ucaseMap = ::ucasemap_open(locale, U_FOLD_CASE_DEFAULT, &error);
				AssertNoError(error, "ucasemap_open");
				return UCaseMapPtr(ucaseMap);
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

		enum class CompareResult : int { Less = -1, Equal = 0 , Greater = +1 };

		inline CompareResult CompareNoCase(const char * s1, size_t s1size, const char * s2, size_t s2size, const char * locale = nullptr)
		{
			auto collator = Detail::MakeCollator(locale); // cache? perf test
			::ucol_setStrength(collator.get(), UCOL_SECONDARY);

			UCharIterator i1, i2;
			::uiter_setUTF8(&i1, s1, static_cast<int32_t>(s1size));
			::uiter_setUTF8(&i2, s2, static_cast<int32_t>(s2size));

			UErrorCode error = U_ZERO_ERROR;
			UCollationResult result = ::ucol_strcollIter(collator.get(), &i1, &i2, &error);
			Detail::AssertNoError(error, "ucol_strcollIter");
			switch (result)
			{
				case UCOL_LESS: return CompareResult::Less;
				case UCOL_EQUAL: return CompareResult::Equal;
				case UCOL_GREATER: return CompareResult::Greater;
				default:
					throw std::runtime_error("Unexpected result");
			}
		}

		inline std::string ToLower(const std::string & value, const char * locale = nullptr)
		{
			UErrorCode error = U_ZERO_ERROR;
			auto map = Detail::MakeUCaseMap(locale); // cache

			const int32_t sourceLength = static_cast<int32_t>(value.size());
			const int32_t destLength = ::ucasemap_utf8ToLower(map.get(), nullptr, 0, value.c_str(), sourceLength, &error);
			Detail::AssertTrue(error== U_BUFFER_OVERFLOW_ERROR, "ucasemap_utf8ToLower");
		
			error = U_ZERO_ERROR;
			Util::StackOrHeap<char, 256> s;
			s.EnsureSize(destLength);
			::ucasemap_utf8ToLower(map.get(), s.Get(), destLength, value.c_str(), sourceLength, &error);
			Detail::AssertNoError(error, "ucasemap_utf8ToLower");
			return { s.Get(), static_cast<size_t>(destLength) };
		}
		
		inline std::string ToUpper(const std::string & value, const char * locale = nullptr)
		{
			UErrorCode error = U_ZERO_ERROR;
			auto map = Detail::MakeUCaseMap(locale); // cache

			const int32_t sourceLength = static_cast<int32_t>(value.size());
			const int32_t destLength = ::ucasemap_utf8ToUpper(map.get(), nullptr, 0, value.c_str(), sourceLength, &error);
			Detail::AssertTrue(error== U_BUFFER_OVERFLOW_ERROR, "ucasemap_utf8ToUpper");
		
			error = U_ZERO_ERROR;
			Util::StackOrHeap<char, 256> s;
			s.EnsureSize(destLength);
			::ucasemap_utf8ToUpper(map.get(), s.Get(), destLength, value.c_str(), sourceLength, &error);
			Detail::AssertNoError(error, "ucasemap_utf8ToUpper");
			return { s.Get(), static_cast<size_t>(destLength) };
		}

		inline CompareResult CompareNoCase(const std::string & s1, const std::string & s2, const char * locale = nullptr)
		{
			return CompareNoCase(s1.c_str(), s1.size(), s2.c_str(), s2.size(), locale);
		}

		template <typename CharType, typename StringType = std::basic_string<CharType>>
		struct NoCaseLess
		{
			bool operator()(const StringType & s1, const StringType & s2) const;
		};

		template <>
		inline bool NoCaseLess<char>::operator()(const std::string & s1, const std::string & s2) const
		{
			return CompareNoCase(s1, s2) == CompareResult::Less;
		}

		template <>
		inline bool NoCaseLess<wchar_t>::operator()(const std::wstring & s1, const std::wstring & s2) const
		{
			return ::_wcsicmp(s1.c_str(), s2.c_str()) < 0;
		}

		template<class CharType> class NoCaseHash
		{
#ifdef _WIN64
			static constexpr size_t fnvPrime = 1099511628211ULL;
#else
			static constexpr size_t fnvPrime = 16777619U;
#endif
		public:
			size_t operator()(const std::basic_string<CharType> & key) const noexcept
			{
				size_t val{};
				for (CharType c : ToLower(key))
				{
					val ^= static_cast<size_t>(c);
					val *= fnvPrime;
				}
				return val;
			}
		};

		template<class CharType> struct NoCaseEquality
		{
			constexpr bool operator()(const std::basic_string<CharType> & left, const std::basic_string<CharType> & right) const
			{
				return CompareNoCase(left, right) == CompareResult::Equal;
			}
		};
	}
}
