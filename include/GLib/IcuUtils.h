#pragma once

#include <GLib/StackOrHeap.h>

#include <memory>
#include <stdexcept>
#include <string>

#ifdef __linux__
#include <unicode/ucasemap.h>
#include <unicode/ucol.h>
#elif _MSC_VER
#include <icu.h>
#pragma comment(lib, "icuuc.lib")
#pragma comment(lib, "icuin.lib")
#endif

namespace GLib::IcuUtils
{
	namespace Detail
	{
		inline void AssertNoError(UErrorCode const error, std::string_view const msg)
		{
			if (U_FAILURE(error) != UBool {})
			{
				throw std::runtime_error(std::string(msg) + " : " + u_errorName(error));
			}
		}

		inline void AssertTrue(bool const value, std::string_view const msg)
		{
			if (!value)
			{
				throw std::runtime_error(std::string(msg));
			}
		}

		struct CollatorCloser
		{
			void operator()(UCollator * const collator) const noexcept
			{
				ucol_close(collator);
			}
		};

		using CollatorPtr = std::unique_ptr<UCollator, CollatorCloser>;

		inline CollatorPtr MakeCollator(char const * const locale = nullptr)
		{
			UErrorCode error = U_ZERO_ERROR;
			UCollator * col = ucol_open(locale, &error);
			AssertNoError(error, "ucol_open");

			// throw if locale is specified but is not used
			AssertTrue(locale == nullptr || error != U_USING_DEFAULT_WARNING, "ucol_open");
			return CollatorPtr(col);
		}

		struct UCaseMapCloser
		{
			void operator()(UCaseMap * const map) const noexcept
			{
				ucasemap_close(map);
			}
		};

		using UCaseMapPtr = std::unique_ptr<UCaseMap, UCaseMapCloser>;

		inline UCaseMapPtr MakeUCaseMap(char const * locale = nullptr)
		{
			UErrorCode error = U_ZERO_ERROR;
			UCaseMap * uCaseMap = ucasemap_open(locale, U_FOLD_CASE_DEFAULT, &error);
			AssertNoError(error, "MakeUCaseMap");
			return UCaseMapPtr(uCaseMap);
		}
	}

	enum class CompareResult : int8_t
	{
		Less = -1,
		Equal = 0,
		Greater = +1
	};

	inline CompareResult CompareNoCase(std::string_view const value1, std::string_view const value2, char const * const locale = nullptr)
	{
		auto const collator = Detail::MakeCollator(locale); // cache? perf test
		ucol_setStrength(collator.get(), UCOL_SECONDARY);

		UCharIterator iter1;
		UCharIterator iter2;

		uiter_setUTF8(&iter1, value1.data(), static_cast<int32_t>(value1.size()));
		uiter_setUTF8(&iter2, value2.data(), static_cast<int32_t>(value2.size()));

		UErrorCode error = U_ZERO_ERROR;
		UCollationResult const result = ucol_strcollIter(collator.get(), &iter1, &iter2, &error);
		Detail::AssertNoError(error, "ucol_strcollIter");
		switch (result)
		{
			case UCOL_LESS:
				return CompareResult::Less;
			case UCOL_EQUAL:
				return CompareResult::Equal;
			case UCOL_GREATER:
				return CompareResult::Greater;
		}
		throw std::runtime_error("Unexpected result");
	}

	inline CompareResult CompareNoCase(std::string_view const value1, std::string_view const value2, size_t const size,
																		 char const * const locale = nullptr)
	{
		return CompareNoCase(value1.substr(0, size), value2.substr(0, size), locale);
	}

	inline std::string ToLower(std::string const & value, char const * const locale = nullptr)
	{
		UErrorCode error = U_ZERO_ERROR;
		auto const map = Detail::MakeUCaseMap(locale); // cache

		auto const sourceLength = static_cast<int>(value.size());
		auto const destLength = ucasemap_utf8ToLower(map.get(), nullptr, 0, value.c_str(), sourceLength, &error);
		Detail::AssertTrue(error == U_BUFFER_OVERFLOW_ERROR, "ucasemap_utf8ToLower");

		error = U_ZERO_ERROR;
		Util::CharBuffer buffer;
		buffer.EnsureSize(destLength);
		ucasemap_utf8ToLower(map.get(), buffer.Get(), destLength, value.c_str(), sourceLength, &error);
		Detail::AssertNoError(error, "ucasemap_utf8ToLower");
		return {buffer.Get(), static_cast<size_t>(destLength)};
	}

	inline std::string ToUpper(std::string const & value, char const * locale = nullptr)
	{
		UErrorCode error = U_ZERO_ERROR;
		auto const map = Detail::MakeUCaseMap(locale); // cache

		auto const sourceLength = static_cast<int>(value.size());
		int const destLength = ucasemap_utf8ToUpper(map.get(), nullptr, 0, value.c_str(), sourceLength, &error);
		Detail::AssertTrue(error == U_BUFFER_OVERFLOW_ERROR, "ucasemap_utf8ToUpper");

		error = U_ZERO_ERROR;
		Util::CharBuffer buffer;
		buffer.EnsureSize(destLength);
		ucasemap_utf8ToUpper(map.get(), buffer.Get(), destLength, value.c_str(), sourceLength, &error);
		Detail::AssertNoError(error, "ucasemap_utf8ToUpper");
		return {buffer.Get(), static_cast<size_t>(destLength)};
	}
}
