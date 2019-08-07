#ifndef FORMATTER_H
#define FORMATTER_H

#include "GLib/Span.h"
#include "GLib/printfformatpolicy.h"

#include <functional>
#include <iomanip>
#include <sstream>

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
			static auto test(...) -> decltype(std::false_type());

			static const bool value = decltype(test<T>(0))::value;
		};

		// todo: use string_view for formats
		using StreamFunction = std::function<void(std::ostream &, const std::string &)>;

		inline void FormatError()
		{
			throw std::logic_error("Invalid format string");
		}

		inline void CheckEmptyFormat(const std::string & format)
		{
			if (!format.empty())
			{
				throw std::logic_error("Unexpected non-empty format : " + format);
			}
		}

		inline std::ostream & AppendFormatHelper(std::ostream & str, const std::string_view & view, const Span<StreamFunction> & args)
		{
			constexpr auto DecimalShift = 10;
			char ch = {};
			for (auto it = view.begin(), end = view.end();;)
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

				size_t index {};
				do
				{
					index = index * DecimalShift + ch - '0';
					++it;
					if (it == end)
					{
						FormatError();
					}
					ch = *it;
				} while (ch >= '0' && ch <= '9');

				while (it != end && (ch = *it) == ' ')
				{
					++it;
				}

				bool leftJustify = {};
				size_t width {};
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
						width = width * DecimalShift + ch - '0';
						++it;
						if (it == end)
						{
							FormatError();
						}
						ch = *it;
					} while (ch >= '0' && ch <= '9');
				}

				while (it != end && (ch = *it) == ' ')
				{
					++it;
				}

				std::string format; // use strng_view?

				if (ch == ':')
				{
					std::ostringstream tmp; // capture a string_view
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

					format = tmp.str();
				}
				if (ch != '}')
				{
					FormatError();
				}
				++it;

				if (width != 0)
				{
					str << (leftJustify ? std::left : std::right) << std::setw(width);
				}

				args[index](str, format);
			}
			return str;
		}
	}

	template <typename Policy>
	class FormatterT
	{
	public:
		template <typename... Ts> static std::ostream & Format(std::ostream & str, const char * format, const Ts&... ts)
		{
			FormatterDetail::StreamFunction ar[]{ ToStreamFunctions(ts)...};
			return FormatterDetail::AppendFormatHelper(str, format, {ar, sizeof...(Ts)});
		}

		template <typename... Ts> static std::string Format(const char * format, Ts&&... ts)
		{
			std::ostringstream str;
			Format(str, format, std::forward<Ts>(ts)...);
			return str.str();
		}

		template <typename... Ts> static std::string Format(const char * format)
		{
			(void)format;
			throw std::logic_error("NoArguments");
		}

	private:
		// ? http://www.drdobbs.com/cpp/efficient-use-of-lambda-expressions-and/232500059
		template <typename T>
		static FormatterDetail::StreamFunction ToStreamFunctions(const T& t)
		{
			return FormatterDetail::StreamFunction([&](std::ostream & stm, const std::string & format)
			{
				FormatImpl(stm, t, format, 0);
			});
		}

		template<class T>
		static auto FormatImpl(std::ostream& os, const T& obj, const std::string & format, int unused)
			-> decltype(Policy::Format(os, obj, format), void())
		{
			(void)unused;
			return Policy::Format(os, obj, format);
		}

		template <typename T, typename std::enable_if<FormatterDetail::IsFormattable<T>::value>::type* = nullptr>
		static void FormatImpl(std::ostream& stm, const T& value, const std::string & format, long unused)
		{
			(void)unused;
			if (!format.empty())
			{
				value.Format(stm, format);
			}
			else
			{
				stm << value;
			}
		}

		template <typename T, typename std::enable_if<!FormatterDetail::IsFormattable<T>::value>::type* = nullptr>
		static void FormatImpl(std::ostream& stm, const T& value, const std::string & format, long unused)
		{
			// handle T==wide string?
			(void)unused;
			FormatterDetail::CheckEmptyFormat(format);
			stm << value;
		}
	};

	using Formatter = FormatterT<FormatterPolicy::Printf>;
}

#endif // FORMATTER_H
