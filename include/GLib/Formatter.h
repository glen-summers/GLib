#pragma once

#include <GLib/PrintfFormatPolicy.h>

#include <array>
#include <functional>
#include <iomanip>
#include <span>
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

			static constexpr bool value = decltype(test<T>(0))::value;
		};

		// todo: use string_view for formats
		using StreamFunction = std::function<void(std::ostream &, std::string const &)>;

		inline void FormatError()
		{
			throw std::logic_error("Invalid format string");
		}

		inline void CheckEmptyFormat(std::string const & format)
		{
			if (!format.empty())
			{
				throw std::logic_error("Unexpected non-empty format : " + format);
			}
		}

		inline std::ostream & AppendFormatHelper(std::ostream & str, std::string_view view, std::span<StreamFunction> const & args)
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

				if (ch = *++it; it == end || (ch < '0' || ch > '9'))
				{
					FormatError();
				}

				size_t index {};
				do
				{
					index = index * DecimalShift + ch - '0';
					if (++it == end)
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
				std::streamsize width {};
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

				std::string format; // use string_view?

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
		template <typename... Ts>
		static std::ostream & Format(std::ostream & str, std::string_view const format, Ts const &... ts)
		{
			std::array<FormatterDetail::StreamFunction, sizeof...(Ts)> ar {ToStreamFunctions(ts)...};
			return FormatterDetail::AppendFormatHelper(str, format, {ar.data(), ar.size()});
		}

		template <typename... Ts>
		static std::string Format(std::string_view const format, Ts &&... ts)
		{
			std::ostringstream str;
			Format(str, format, std::forward<Ts>(ts)...);
			return str.str();
		}

		template <typename... Ts>
		static std::string Format(std::string_view const format)
		{
			static_cast<void>(format);
			throw std::logic_error("NoArguments");
		}

	private:
		// ? http://www.drdobbs.com/cpp/efficient-use-of-lambda-expressions-and/232500059
		template <typename T>
		static FormatterDetail::StreamFunction ToStreamFunctions(T const & t)
		{
			return FormatterDetail::StreamFunction([&](std::ostream & stm, std::string const & format) { FormatImpl(stm, t, format, 0); });
		}

		template <typename T>
		static auto FormatImpl(std::ostream & os, T const & obj, std::string const & format, int const unused)
			-> decltype(Policy::Format(os, obj, format), void())
		{
			static_cast<void>(unused);
			return Policy::Format(os, obj, format);
		}

		template <typename T, std::enable_if_t<FormatterDetail::IsFormattable<T>::value> * = nullptr>
		static void FormatImpl(std::ostream & stm, T const & value, std::string const & format, long const unused)
		{
			static_cast<void>(unused);
			if (!format.empty())
			{
				value.Format(stm, format);
			}
			else
			{
				stm << value;
			}
		}

		template <typename T, std::enable_if_t<!FormatterDetail::IsFormattable<T>::value> * = nullptr>
		static void FormatImpl(std::ostream & stm, T const & value, std::string const & format, long const unused)
		{
			// handle T==wide string?
			static_cast<void>(unused);
			FormatterDetail::CheckEmptyFormat(format);
			stm << value; // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
		}
	};

	using Formatter = FormatterT<FormatterPolicy::Printf>;
}
