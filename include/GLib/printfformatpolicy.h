#ifndef PRINTF_FORMAT_POLICY_H
#define PRINTF_FORMAT_POLICY_H

#include <ostream>

#include "stackorheap.h"
#include "compat.h"

namespace GLib
{
	namespace FormatterPolicy
	{
		namespace Detail
		{
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
				const int len = ::snprintf(nullptr, 0, f, value);
				if (len < 0)
				{
					Compat::StrError("snprintf failed");
				}
				s.EnsureSize(len + 1);
				::snprintf(s.Get(), s.size(), f, value);

				stm << s.Get();
			}

			template<unsigned int> void FormatPointer(std::ostream & stm, void * const & value)
			{
				static_assert("Unknown pointer size");
			}

			template<>
			inline void FormatPointer<4>(std::ostream & stm, void * const & value)
			{
				ToStringImpl("", stm, value, "%08x");
			}

			template<>
			inline void FormatPointer<8>(std::ostream & stm, void * const & value)
			{
				ToStringImpl("", stm, value, "%016x");
			}
		}

		class Printf
		{
		public:
			static void Format(std::ostream & stm, const char & value, const std::string & format)
			{
				Detail::ToStringImpl("%c", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned char & value, const std::string & format)
			{
				Detail::ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, const short & value, const std::string & format)
			{
				Detail::ToStringImpl("%d", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned short & value, const std::string & format)
			{
				Detail::ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, const int & value, const std::string & format)
			{
				Detail::ToStringImpl("%d", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned int & value, const std::string & format)
			{
				Detail::ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, const long & value, const std::string & format)
			{
				Detail::ToStringImpl("%ld", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned long & value, const std::string & format)
			{
				Detail::ToStringImpl("%lu", stm, value, format);
			}

			static void Format(std::ostream & stm, const long long & value, const std::string & format)
			{
				Detail::ToStringImpl("%lld", stm, value, format);
			}

			static void Format(std::ostream & stm, const unsigned long long & value, const std::string & format)
			{
				Detail::ToStringImpl("%llu", stm, value, format);
			}

			static void Format(std::ostream & stm, const float & value, const std::string & format)
			{
				Detail::ToStringImpl("%g", stm, value, format);
			}

			static void Format(std::ostream & stm, const double & value, const std::string & format)
			{
				Detail::ToStringImpl("%g", stm, value, format);
			}

			static void Format(std::ostream & stm, const long double & value, const std::string & format)
			{
				Detail::ToStringImpl("%Lg", stm, value, format);
			}

			static void Format(std::ostream & stm, void * const & value, const std::string & format)
			{
				// format appears to have no affect on %p windows gets %0p, linux gets %#p
				//ToStringImpl("%p", stm, value, format);
				if (format.empty())
				{
					Detail::FormatPointer<sizeof(void*)>(stm, value);
					// or << std::hex << << std::noshowbase << std::left << std::setfill('0') << std::setw(sizeof(value)/4) << value
				}
				else
				{
					Detail::ToStringImpl("", stm, value, format);
				}
			}

		private:
			template< typename T> static void Format(std::ostream & stm, const T & value, const std::string & fmt);
		};
	}
}

#endif // PRINTF_FORMAT_POLICY_H
