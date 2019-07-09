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
}
#define BOOST_CHECK_EXCEPTION_EX( S, E, P, C )\
BOOST_CHECK_THROW_IMPL( S, E, P<E>( ex, C ), \
": validation on the raised exception through predicate \"" BOOST_STRINGIZE(P<E>) "\"", CHECK )

#define GLIB_CHECK_EXCEPTION( S, E, C ) BOOST_CHECK_EXCEPTION_EX( S, E, TestUtils::ExpectException, C )

#define GLIB_CHECK_RUNTIME_EXCEPTION( S, C ) GLIB_CHECK_EXCEPTION(S, std::runtime_error, C)
