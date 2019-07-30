#pragma once

#include "GLib/cvt.h"

#include <iostream>

namespace TestUtils
{
	template <typename T>
	bool ExpectException(const T & e, const std::string & what)
	{
		bool compare = e.what() == what;
		if (!compare)
		{
			std:: wcerr << "Expected exception message: \"" << GLib::Cvt::a2w(what) << "\" got: \"" << GLib::Cvt::a2w(e.what()) << "\"" << std::endl;
		}
		return compare;
	}

	template <typename Iterator>
	inline void Dump(std::ostream & s, Iterator begin, Iterator end, unsigned count)
	{
		if (begin == end)
		{
			return;
		}

		for (auto it = begin; it!=end && count != 0; ++it, --count)
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

	inline void Compare(const std::string & value, const std::string & expected, unsigned length = 30)
	{
		auto ret = std::mismatch(value.begin(), value.end(), expected.begin(), expected.end());
		std::ostringstream s;

		if (ret.first != value.end())
		{
			size_t pos = std::distance(value.begin(), ret.first);
			s << "Difference at position: " << pos << " [";
			Dump(s, ret.first, value.end(), length);
			s << "]";
		}
		else if (ret.second != expected.end())
		{
			s << "Expected data at end missing: [";
			Dump(s, ret.second, expected.end(), length);
			s << "]";
		}

		if (!s.str().empty())
		{
			throw std::runtime_error(s.str());
		}
	}
}

#define BOOST_CHECK_EXCEPTION_EX( S, E, P, C )\
BOOST_CHECK_THROW_IMPL( S, E, P<E>( ex, C ), \
": validation on the raised exception through predicate \"" BOOST_STRINGIZE(P<E>) "\"", CHECK )
#define GLIB_CHECK_EXCEPTION( S, E, C ) BOOST_CHECK_EXCEPTION_EX( S, E, TestUtils::ExpectException, C )
#define GLIB_CHECK_RUNTIME_EXCEPTION( S, C ) GLIB_CHECK_EXCEPTION(S, std::runtime_error, C)
