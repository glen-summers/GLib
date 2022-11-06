#pragma once

#include <GLib/Cvt.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <optional>

namespace TestUtils
{
	constexpr auto Tab = 9;
	constexpr auto NewLine = 10;
	constexpr auto Space = 32;
	constexpr auto DefaultCompareSize = 32;

	template <typename T>
	bool ExpectException(const T & e, const std::string & what)
	{
		bool compare = e.what() == what;
		if (!compare)
		{
			std::wcerr << "Expected exception message: \"" << GLib::Cvt::A2W(what) << "\" got: \"" << GLib::Cvt::A2W(e.what()) << "\"" << std::endl;
		}
		return compare;
	}

	template <typename Iterator>
	void Dump(std::ostream & s, Iterator begin, Iterator end, size_t maxCount)
	{
		s << '[';
		if (begin != end)
		{
			for (auto it = begin; it != end && maxCount != 0; ++it, --maxCount)
			{
				switch (*it)
				{
					// hex?
					case Tab:
						s << "\\t";
						break;

					case NewLine:
						s << "\\n";
						break;

					case Space:
						s << "\\s";
						break;

					default:
						s << *it;
						break;
				}
			}
		}
		s << ']';
	}

	inline bool CompareStrings(const std::string & expected, const std::string & actual, size_t maxCount, std::ostringstream & msg)
	{
		auto ret = std::mismatch(expected.begin(), expected.end(), actual.begin(), actual.end());

		bool result {};
		if (ret.second != actual.end())
		{
			auto pos = std::distance(actual.begin(), ret.second);
			msg << "Difference at position: " << pos << "\n";
			msg << "Expected: ";
			Dump(msg, ret.first, expected.end(), maxCount);
			msg << '\n';

			msg << "Actual  : ";
			Dump(msg, actual.begin() + pos, actual.end(), maxCount);
			result = true;
		}
		else if (ret.first != expected.end())
		{
			msg << "Expected data at end missing: ";
			Dump(msg, ret.first, expected.end(), maxCount);
			result = true;
		}

		return result;
	}

	inline void Compare(const std::string & expected, const std::string & actual, size_t maxCount = DefaultCompareSize)
	{
		std::ostringstream stm;
		if (CompareStrings(expected, actual, maxCount, stm))
		{
			throw std::runtime_error(stm.str());
		}
	}

	template <typename T>
	constexpr auto ToInt(std::string_view value)
	{
		constexpr auto Ten = 10;
		T accumulator {};

		for (char c : value)
		{
			if (c < '0' || c > '9')
			{
				throw std::runtime_error("Not a Digit");
			}

			// overflow?
			accumulator *= Ten;
			accumulator += c - '0';
		}

		return accumulator;
	}

	template <typename T>
	constexpr T ToFloat(std::string_view value)
	{
		constexpr T Ten = 10;
		T accumulator1 {};
		T accumulator2 {};
		size_t decimals {};
		std::optional<size_t> point;

		for (char c : value)
		{
			if (c == '.')
			{
				if (point.has_value())
				{
					throw std::runtime_error("Multiple '.'s");
				}
				point = true;
				continue;
			}

			if (c < '0' || c > '9')
			{
				throw std::runtime_error("Not a Digit");
			}

			if (!point)
			{
				accumulator1 *= Ten;
				accumulator1 += c - '0';
			}
			else
			{
				accumulator2 *= Ten;
				accumulator2 += c - '0';
				++decimals;
			}
		}

		return static_cast<T>(accumulator1 + (accumulator2 / std::pow(Ten, decimals)));
	}

	template <typename T>
	constexpr auto ToHex(std::string_view value)
	{
		constexpr auto Ten = 10;
		constexpr auto HexTen = 0x10;

		T accumulator {};
		T val {};

		for (char c : value)
		{
			if (c >= '0' && c <= '9')
			{
				val = c - '0';
			}
			else if (c >= 'A' && c <= 'F')
			{
				val = c - 'A' + Ten;
			}
			else if (c >= 'a' && c <= 'f')
			{
				val = c - 'a' + Ten;
			}
			else
			{
				throw std::runtime_error("Not a Hex Digit");
			}

			// overflow?
			accumulator *= HexTen;
			accumulator += val;
		}

		return accumulator;
	}
}

#define I8(x) TestUtils::ToInt<int8_t>(#x)
#define I16(x) TestUtils::ToInt<int16_t>(#x)
#define I32(x) TestUtils::ToInt<int32_t>(#x)
#define I64(x) TestUtils::ToInt<int64_t>(#x)

#define U8(x) TestUtils::ToInt<uint8_t>(#x)
#define U16(x) TestUtils::ToInt<uint16_t>(#x)
#define U32(x) TestUtils::ToInt<uint32_t>(#x)
#define U64(x) TestUtils::ToInt<uint64_t>(#x)

#define H16(x) TestUtils::ToHex<uint16_t>(#x)
#define H32(x) TestUtils::ToHex<uint32_t>(#x)
#define H64(x) TestUtils::ToHex<uint64_t>(#x)

#define SZ(x) TestUtils::ToInt<size_t>(#x)

#define FL(x) TestUtils::ToFloat<float>(#x)
#define DB(x) TestUtils::ToFloat<double>(#x)
#define LD(x) TestUtils::ToFloat<long double>(#x)

// ReSharper disable All
namespace boost::test_tools::tt_detail
{
	template <typename K, typename V>
	struct print_log_value<std::pair<K, V>>
	{
		void operator()(std::ostream & str, const std::pair<K, V> & item)
		{
			str << '{' << item.first << ',' << item.second << '}';
		}
	};

	template <>
	struct print_log_value<std::wstring>
	{
		void operator()(std::ostream & str, const std::wstring & item)
		{
			str << GLib::Cvt::W2A(item);
		}
	};
}

// clang-format off
#define /*NOLINT*/ BOOST_CHECK_EXCEPTION_EX(Src, Ex, Pred, C)\
	BOOST_CHECK_THROW_IMPL(Src, Ex, Pred<Ex>(ex, C), ": validation on the raised exception through predicate \"" BOOST_STRINGIZE(Pred<Ex>) "\"", CHECK) // NOLINT

#define GLIB_CHECK_EXCEPTION(Src, E, C) BOOST_CHECK_EXCEPTION_EX(Src, E, TestUtils::ExpectException, C)
#define GLIB_CHECK_RUNTIME_EXCEPTION(Src, C) GLIB_CHECK_EXCEPTION(Src, std::runtime_error, C)
#define GLIB_CHECK_LOGIC_EXCEPTION(Src, C) GLIB_CHECK_EXCEPTION(Src, std::logic_error, C)

#define AUTO_TEST_SUITE(x) BOOST_AUTO_TEST_SUITE(x)																						// NOLINT
#define AUTO_TEST_SUITE_END BOOST_AUTO_TEST_SUITE_END																					// NOLINT
#define AUTO_TEST_CASE(x) BOOST_AUTO_TEST_CASE(x)																							// NOLINT
#define TEST(x) BOOST_TEST(x)																																	// NOLINT
#define CHECK_EXCEPTION(Src, Ex, Pred) BOOST_CHECK_EXCEPTION(Src, Ex, Pred)										// NOLINT
#define CHECK(Pred) BOOST_CHECK(Pred)																													// NOLINT
#define FAIL(Msg) BOOST_FAIL(Msg)																															// NOLINT
#define CHECK_EQUAL_COLLECTIONS(Lb, Le, Rb, Re) BOOST_CHECK_EQUAL_COLLECTIONS(Lb, Le, Rb, Re)	// NOLINT
// clang-format on
// ReSharper restore All