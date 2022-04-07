#pragma once

#include <GLib/Cvt.h>

#include <algorithm>
#include <iostream>

namespace TestUtils
{
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
					case 9:
						s << "\\t";
						break;
					case 10:
						s << "\\n";
						break;
					case 32:
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
			size_t pos = std::distance(actual.begin(), ret.second);
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

	inline void Compare(const std::string & expected, const std::string & actual, size_t maxCount = 30)
	{
		std::ostringstream stm;
		if (CompareStrings(expected, actual, maxCount, stm))
		{
			throw std::runtime_error(stm.str());
		}
	}
}

// ReSharper disable All
namespace boost::test_tools::tt_detail
{
	template <typename K, typename V>
	struct print_log_value<std::pair<K, V>>
	{
		inline void operator()(std::ostream & str, const std::pair<K, V> & item)
		{
			str << '{' << item.first << ',' << item.second << '}';
		}
	};

	template <>
	struct print_log_value<std::wstring>
	{
		inline void operator()(std::ostream & str, const std::wstring & item)
		{
			str << GLib::Cvt::W2A(item);
		}
	};
}

#define BOOST_CHECK_EXCEPTION_EX(S, E, P, C)                                                                                                         \
	BOOST_CHECK_THROW_IMPL(S, E, P<E>(ex, C), ": validation on the raised exception through predicate \"" BOOST_STRINGIZE(P<E>) "\"", CHECK)
#define GLIB_CHECK_EXCEPTION(S, E, C) BOOST_CHECK_EXCEPTION_EX(S, E, TestUtils::ExpectException, C)
#define GLIB_CHECK_RUNTIME_EXCEPTION(S, C) GLIB_CHECK_EXCEPTION(S, std::runtime_error, C)
#define GLIB_CHECK_LOGIC_EXCEPTION(S, C) GLIB_CHECK_EXCEPTION(S, std::logic_error, C)
// ReSharper restore All