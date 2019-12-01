#ifndef PRINTF_FORMAT_POLICY_H
#define PRINTF_FORMAT_POLICY_H

#include <GLib/compat.h>
#include <GLib/cvt.h>
#include <GLib/stackorheap.h>

#include <iomanip>
#include <ostream>

namespace GLib
{
	// move?
	struct Money
	{
		long double value;
	};

	namespace FormatterPolicy
	{
		namespace Detail
		{
			inline std::string CheckFormat(const char * defaultFormat, const std::string & format)
			{
				std::string f = format.empty() ? defaultFormat : format;
				if (*f.begin() != '%')
				{
					throw std::logic_error("Invalid format : " + f);
				}
				return f;
			}

			inline void CheckFormatEmpty(const std::string & format)
			{
				if (!format.empty())
				{
					throw std::logic_error("Invalid format : " + format);
				}
			}

			template <typename T>
			static void ToStringImpl(const char * defaultFormat, std::ostream & stm, const T & value, const std::string & format)
			{
				std::string f = CheckFormat(defaultFormat, format);
				constexpr auto InitialBufferSize = 21;
				Util::StackOrHeap<char, InitialBufferSize> s;
				const int len = ::snprintf(nullptr, 0, f.c_str(), value); // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg) by design
				Compat::AssertTrue(len >= 0, "snprintf", errno);
				s.EnsureSize(static_cast<size_t>(len) + 1);
				::snprintf(s.Get(), s.size(), f.c_str(), value); // NOLINT(cppcoreguidelines-pro-type-vararg,hicpp-vararg) by design

				stm << s.Get();
			}

			template <>
			inline void ToStringImpl(const char * defaultFormat, std::ostream & stm, const std::tm & value, const std::string & format)
			{
				std::string f = CheckFormat(defaultFormat, format);
				// stream to wide to correctly convert locale symbols, is there a better better way? maybe when code convert gets fixed
				std::wstringstream wideStream;
				(void)wideStream.imbue(stm.getloc());
				wideStream << std::put_time(&value, Cvt::a2w(f).c_str());
				stm << Cvt::w2a(wideStream.str());
			}

			template <>
			inline void ToStringImpl(const char * defaultFormat, std::ostream & stm, const Money & value, const std::string & format)
			{
				(void)defaultFormat;
				CheckFormatEmpty(format);
				// stream to wide to correctly convert locale symbols, is there a better better way? maybe when code convert gets fixed
				std::wstringstream wideStream;
				(void)wideStream.imbue(stm.getloc());
				wideStream << std::showbase << std::put_money(value.value);
				stm << Cvt::w2a(wideStream.str());
			}

			template<size_t> void FormatPointer(std::ostream & stm, void * const & value)
			{
				(void)value;
				throw std::runtime_error("Unknown pointer size : " + std::to_string(sizeof(value)));
			}

			template<>
			inline void FormatPointer<sizeof(uint32_t)>(std::ostream & stm, void * const & value)
			{
				ToStringImpl("", stm, value, "%08x");
			}

			template<>
			inline void FormatPointer<sizeof(uint64_t)>(std::ostream & stm, void * const & value)
			{
				ToStringImpl("", stm, value, "%016llx");
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
				if (format.empty())
				{
					Detail::FormatPointer<sizeof(void*)>(stm, value);
				}
				else
				{
					Detail::ToStringImpl("", stm, value, format);
				}
			}

			// actually not printf calls, move to a shared c++ policy?
			static void Format(std::ostream & stm, const std::tm & value, const std::string & format)
			{
				Detail::ToStringImpl("%d %b %Y, %H:%M:%S", stm, value, format);
			}

			static void Format(std::ostream & stm, const Money & value, const std::string & format)
			{
				// assert format is empty
				Detail::ToStringImpl("", stm, value, format);
			}

		private:
			template< typename T> static void Format(std::ostream & stm, const T & value, const std::string & fmt);
		};
	}
}

#endif // PRINTF_FORMAT_POLICY_H
