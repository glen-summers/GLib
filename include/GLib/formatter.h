#ifndef FORMATTER_H
#define FORMATTER_H

#include "GLib/printfformatpolicy.h"

#include <sstream>
#include <functional>
#include <iomanip>
#include <array>

// namespace Formatter?

namespace GLib
{
	namespace FormatterDetail
	{
		template <typename T>
		struct IsFormattable
		{
			template <typename C>
			static auto test(int) -> decltype(&C::Format, std::true_type());

			template <typename>
			static auto test(...) -> std::false_type;

			static const bool value = decltype(test<T>(0))::value;
		};

		using StreamFunction = std::function<void(std::ostream&, std::string&)>;

		inline void FormatError()
		{
			throw std::logic_error("Invalid format string");
		}

		inline void CheckEmptyFormat(const std::string &fmt)
		{
			if (!fmt.empty())
			{
					throw std::logic_error("Unexpected non-empty format : " + fmt);
			}
		}

		// back inline again, RETODO, move out of line if Glib linked in
		// or not bother as will prob be out of lined by compiler anyway
		// measure impact on multiple copies in translation units?
		// now using template for args using std::array to avoid pointyer arithmetic, move to c++20 std::span
		// try and avoid Args template param to avoid multiple instance code bloat!
		// try use a span?
		template <typename Args>
		inline void AppendFormatHelper(std::ostream & str, const char * format, const Args & args)
		{
			if (!format)
			{
				throw std::logic_error("format cannot be null");
			}

			auto view = std::string_view { format, ::strlen(format) };
			auto it = view.begin(), end = view.end();

			for (char ch = {};;)
			{
				while (it != end)
				{
					ch = *it++;

					if (ch == '}')
					{
						if (it != end && *it == '}') // Treat as escape character for }}
						{
							++it;
						}
						else
						{
							FormatError();
						}
					}

					if (ch == '{')
					{
						if (it != end && *it == '{') // Treat as escape character for {{
						{
							++it;
						}
						else
						{
							--it;
							break;
						}
					}

					str << ch;
				}

				if (it == end)
				{
					break;
				}
				++it;
				if (it == end || (ch = *it) < '0' || ch > '9')
				{
					FormatError();
				}

				unsigned int index {};
				do
				{
					index = index * 10 + ch - '0';
					++it;
					if (it == end)
					{
						FormatError();
					}
					ch = *it;
				} while (ch >= '0' && ch <= '9' && index < 1000000);

				while (it != end && (ch = *it) == ' ')
				{
					++it;
				}

				bool leftJustify = false;
				int width = 0;
				if (ch == ',')
				{
					++it;
					while (it != end && *it == ' ')
					{
						++it;
					}

					if (it == end)
					{
						FormatError();
					}

					ch = *it;
					if (ch == '-')
					{
						leftJustify = true;
						++it;
						if (it == end)
						{
							FormatError();
						}
						ch = *it;
					}

					if (ch < '0' || ch > '9')
					{
						FormatError();
					}

					do
					{
						width = width * 10 + ch - '0';
						++it;
						if (it == end)
						{
							FormatError();
						}
						ch = *it;
					} while (ch >= '0' && ch <= '9' && width < 1000000);
				}

				while (it != end && (ch = *it) == ' ')
				{
					++it;
				}

				if (index >= args.size())
				{
					throw std::logic_error("IndexOutOfRange"); // add extra info
				}

				const StreamFunction & arg = args[index];
				std::string fmt;

				if (ch == ':')
				{
					std::ostringstream tmp;
					++it;
					for (;;)
					{
						if (it == end)
						{
							FormatError();
						}
						ch = *it;
						++it;
						if (ch == '{')
						{
							if (it != end && *it == '{') // Treat as escape character for {{
							{
								++it;
							}
							else
							{
								FormatError();
							}
						}
						else if (ch == '}')
						{
							if (it != end && *it == '}') // Treat as escape character for }}
							{
								++it;
							}
							else
							{
								--it;
								break;
							}
						}

						tmp << ch;
					}

					fmt = tmp.str();
				}
				if (ch != '}')
				{
					FormatError();
				}
				++it;

				//int pad = width - s.length();
				if (width > 0)
				{
					str << (leftJustify ? std::left : std::right) << std::setw(width);
				}

				arg(str, fmt);
			}
		}

		/*struct NumberFormatter
		{
			bool specifierIsUpper = true;
			char specifier = 'G';
			bool isCustomFormat = false;
			int precision = 0;

			NumberFormatter(const std::string & format)
			{
				if (!format.empty())
				{
					char c = format[0];
					if (c >= 'a' && c <= 'z')
					{
						specifier = (char)(c - 'a' + 'A');
						specifierIsUpper = false;
					}
					else if (c < 'A' || c > 'Z')
					{
						isCustomFormat = true;
						specifier = '0';
						return;
					}
					if (format.length() > 1)
					{
						precision = ParsePrecision(format);
						if (precision == -2)
						{
							isCustomFormat = true;
							specifier = '0';
							precision = -1;
						}
					}
				}
			}

			static int ParsePrecision(const std::string & format)
			{
				int precision = 0;
				for (char c : format)
				{
					int val = c - '0';
					precision = precision * 10 + val;
					if (val < 0 || val > 9 || precision > 99)
					{
							return -2;
					}
				}
				return precision;
			}
		};*/
	}

	template <typename Policy>
	class FormatterT
	{
	public:
		template <typename... Ts> static std::ostream & Format(std::ostream & str, const char * format, const Ts&... ts)
		{
			std::array<FormatterDetail::StreamFunction, sizeof...(Ts)> arr = { ToStreamFunctions(ts)...};
			FormatterDetail::AppendFormatHelper(str, format, arr);

			// how about impl as an iterator that has index and format(+left\right pad)?
			// then can call ar[index](str, fmt) here? and possibly avoid std::function?
			return str;
		}

		template <typename... Ts> static std::string Format(const char * format, Ts&&... ts)
		{
			std::ostringstream str;
			Format(str, format, std::forward<Ts>(ts)...);
			return str.str();
		}

	private:
		// ? http://www.drdobbs.com/cpp/efficient-use-of-lambda-expressions-and/232500059
		template <typename T>
		static FormatterDetail::StreamFunction ToStreamFunctions(const T& t)
		{
			return FormatterDetail::StreamFunction([&](std::ostream& stm, const std::string &fmt)
			{
				FormatImpl(stm, t, fmt, 0);
			});
		}

		template<class T>
		static auto FormatImpl(std::ostream& os, const T& obj, const std::string &fmt, int unused)
			-> decltype(Policy::Format(os, obj, fmt), void())
		{
			(void)unused;
				return Policy::Format(os, obj, fmt);
		}

		template <typename T, typename std::enable_if<FormatterDetail::IsFormattable<T>::value>::type* = nullptr>
		static void FormatImpl(std::ostream& stm, const T& value, const std::string &fmt, long unused)
		{
			(void)unused;
				if (!fmt.empty())
				{
					value.Format(stm, fmt);
				}
				else
				{
					stm << value;
				}
		}

		template <typename T, typename std::enable_if<!FormatterDetail::IsFormattable<T>::value>::type* = nullptr>
		static void FormatImpl(std::ostream& stm, const T& value, const std::string &fmt, long unused)
		{
			// handle T==wide string?
			(void)unused;
			FormatterDetail::CheckEmptyFormat(fmt);
			stm << value;
		}
	};

	using Formatter = FormatterT<FormatterPolicy::Printf>;
}

#endif // FORMATTER_H
