#ifndef PRINTFFORMATPOLICY_H
#define PRINTFFORMATPOLICY_H

#include <ostream>
#include <iomanip>

#include "stackorheap.h"
#include "compat.h"

namespace GLib
{
	namespace FormatterPolicy
	{
		class Printf
		{
		public:
			// primitive overloads, these handle case where format is empty
			static void Format(std::ostream & stm, const char & value, const std::string & format)
			{
				ToStringImpl("%c", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned char & value, const std::string & format)
			{
				ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, const short & value, const std::string & format)
			{
				ToStringImpl("%d", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned short & value, const std::string & format)
			{
				ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, const int & value, const std::string & format)
			{
				ToStringImpl("%d", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned int & value, const std::string & format)
			{
				ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, const long & value, const std::string & format)
			{
				ToStringImpl("%ld", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned long & value, const std::string & format)
			{
				ToStringImpl("%lu", stm, value, format);
			}

			static void Format(std::ostream & stm, const long long & value, const std::string & format)
			{
				ToStringImpl("%lld", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned long long & value, const std::string & format)
			{
				ToStringImpl("%llu", stm, value, format);
			}

			static void Format(std::ostream & stm, const float & value, const std::string & format)
			{
				ToStringImpl("%g", stm, value, format);
			}

			static void Format(std::ostream & stm, const double & value, const std::string & format)
			{
				ToStringImpl("%g", stm, value, format);
			}

			static void Format(std::ostream & stm, const long double & value, const std::string & format)
			{
				ToStringImpl("%g", stm, value, format);
			}

			static void Format(std::ostream & stm, void * const & value, const std::string & format)
			{
				ToStringImpl("%#p", stm, value, format);
			}

			static void Format(std::ostream & stm, const std::tm & value, const std::string & fmt)
			{
				stm << std::put_time(&value, fmt.empty() ? "%c" : fmt.c_str());
			}

		private:
			// no conversions!
			template< typename T>
			static void Format(std::ostream & stm, const T & value, const std::string & fmt);

			template <typename T>
			static void ToStringImpl(const char * defaultFormat, std::ostream & stm, const T & value, const std::string & format)
			{
				//NumberFormatter n(format);
				// %[flags][width][.precision][length]specifier
				// %.*x
				const char * f = format.empty() ? defaultFormat : format.c_str();
				if (*f != '%')
				{
					throw std::logic_error("Invalid format : " + format);
				}

				Util::StackOrHeap<char, 21> s;
				int len = ::snprintf(nullptr, 0, f, value);
				if (len < 0)
				{
					Compat::StrError();
				}
				s.EnsureSize(len + 1);
				::snprintf(s.Get(), s.size(), f, value);

				stm << s.Get();
			}
		};
	}
}

#endif // PRINTFFORMATPOLICY_H
