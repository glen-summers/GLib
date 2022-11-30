#pragma once

#include <GLib/Compat.h>
#include <GLib/Cvt.h>
#include <GLib/StackOrHeap.h>

#include <iomanip>
#include <ostream>

namespace GLib
{
	// move?
	struct Money
	{
		long double Value;
	};

	namespace FormatterPolicy
	{
		namespace Detail
		{
			inline std::string CheckFormat(char const * defaultFormat, std::string const & format, std::string const & type)
			{
				std::string f = format.empty() ? defaultFormat : format;
				if (*f.begin() != '%')
				{
					throw std::logic_error("Invalid format : '" + f + "' for Type: " + type);
				}
				return f;
			}

			inline void CheckFormatEmpty(std::string const & format)
			{
				if (!format.empty())
				{
					throw std::logic_error("Invalid format : " + format);
				}
			}

			template <typename T>
			void ToStringImpl(char const * defaultFormat, std::ostream & stm, T const & value, std::string const & format)
			{
				std::string const f = CheckFormat(defaultFormat, format, Compat::Unmangle(typeid(T).name()));
				constexpr auto initialBufferSize = 21;
				Util::StackOrHeap<char, initialBufferSize> s;
				int const len = snprintf(nullptr, 0, f.c_str(), value); // NOLINT until c++/20 Format impl
				Compat::AssertTrue(len >= 0, "ToString", errno);
				s.EnsureSize(static_cast<size_t>(len) + 1);
				snprintf(s.Get(), s.Size(), f.c_str(), value); // NOLINT until c++/20 Format impl

				stm << s.Get();
			}

			template <>
			inline void ToStringImpl(char const * defaultFormat, std::ostream & stm, std::tm const & value, std::string const & format)
			{
				std::string const f = CheckFormat(defaultFormat, format, "tm");
				// stream to wide to correctly convert locale symbols, is there a better better way? maybe when code convert gets fixed
				std::wstringstream wideStream;
				static_cast<void>(wideStream.imbue(stm.getloc()));
				wideStream << std::put_time(&value, Cvt::A2W(f).c_str());
				stm << Cvt::W2A(wideStream.str());
			}

			template <>
			inline void ToStringImpl(char const * defaultFormat, std::ostream & stm, Money const & value, std::string const & format)
			{
				static_cast<void>(defaultFormat);
				CheckFormatEmpty(format);
				// stream to wide to correctly convert locale symbols, is there a better better way? maybe when code convert gets fixed
				std::wstringstream wideStream;
				static_cast<void>(wideStream.imbue(stm.getloc()));
				wideStream << std::showbase << std::put_money(value.Value);
				stm << Cvt::W2A(wideStream.str());
			}

			template <size_t>
			void FormatPointer(std::ostream & stm, void * const & value)
			{
				static_cast<void>(stm);
				static_cast<void>(value);
				throw std::runtime_error("Unknown pointer size : " + std::to_string(sizeof value));
			}

			template <>
			inline void FormatPointer<sizeof(uint32_t)>(std::ostream & stm, void * const & value)
			{
				ToStringImpl("", stm, value, "%08X");
			}

			template <>
			inline void FormatPointer<sizeof(uint64_t)>(std::ostream & stm, void * const & value)
			{
				ToStringImpl("", stm, value, "%016llX");
			}
		}

		class Printf
		{
		public:
			static void Format(std::ostream & stm, char const & value, std::string const & format)
			{
				Detail::ToStringImpl("%c", stm, value, format);
			}

			static void Format(std::ostream & stm, unsigned char const & value, std::string const & format)
			{
				Detail::ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, short const & value, std::string const & format)
			{
				Detail::ToStringImpl("%d", stm, value, format);
			}

			static void Format(std::ostream & stm, unsigned short const & value, std::string const & format)
			{
				Detail::ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, int const & value, std::string const & format)
			{
				Detail::ToStringImpl("%d", stm, value, format);
			}

			static void Format(std::ostream & stm, unsigned int const & value, std::string const & format)
			{
				Detail::ToStringImpl("%u", stm, value, format);
			}

			static void Format(std::ostream & stm, long const & value, std::string const & format)
			{
				Detail::ToStringImpl("%ld", stm, value, format);
			}

			static void Format(std::ostream & stm, unsigned long const & value, std::string const & format)
			{
				Detail::ToStringImpl("%lu", stm, value, format);
			}

			static void Format(std::ostream & stm, long long const & value, std::string const & format)
			{
				Detail::ToStringImpl("%lld", stm, value, format);
			}

			static void Format(std::ostream & stm, unsigned long long const & value, std::string const & format)
			{
				Detail::ToStringImpl("%llu", stm, value, format);
			}

			static void Format(std::ostream & stm, float const & value, std::string const & format)
			{
				Detail::ToStringImpl("%g", stm, value, format);
			}

			static void Format(std::ostream & stm, double const & value, std::string const & format)
			{
				Detail::ToStringImpl("%g", stm, value, format);
			}

			static void Format(std::ostream & stm, long double const & value, std::string const & format)
			{
				Detail::ToStringImpl("%Lg", stm, value, format);
			}

			static void Format(std::ostream & stm, void * const & value, std::string const & format)
			{
				if (format.empty())
				{
					Detail::FormatPointer<sizeof(void *)>(stm, value);
				}
				else
				{
					Detail::ToStringImpl("", stm, value, format);
				}
			}

			// actually not printf calls, move to a shared c++ policy?
			static void Format(std::ostream & stm, std::tm const & value, std::string const & format)
			{
				Detail::ToStringImpl("%d %b %Y, %H:%M:%S", stm, value, format);
			}

			static void Format(std::ostream & stm, Money const & value, std::string const & format)
			{
				// assert format is empty
				Detail::ToStringImpl("", stm, value, format);
			}

		private:
			template <typename T>
			static void Format(std::ostream & stm, T const & value, std::string const & fmt);
		};
	}
}
