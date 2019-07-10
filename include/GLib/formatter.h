#ifndef FORMATTER_H
#define FORMATTER_H

#include "GLib/printfformatpolicy.h"

#include <sstream>
#include <functional>
#include <iomanip>

// namespace Formatter?

namespace GLib
{
	namespace FormatterDetail
	{
			template <typename T>
			struct IsFormattable
			{
					typedef char yes[1];
					typedef char no[2];

					template <typename C>
					static yes& test(decltype(&C::Format));

					template <typename>
					static no& test(...);

					static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
			};

			//template <typename T>
			//struct IsFormattableSig
			//{
			//	typedef char yes[1];
			//	typedef char no[2];

			//	template<typename U> struct Sig
			//	{
			//		// get C2825	'U': must be a class or namespace when followed by '::' with ints, compiler error?
			//		//typedef std::string(U::*fptr)(int);
			//		typedef void (U::*fptr)(std::ostream & s, const std::string & fmt);
			//	};

			//	template<typename SignatureType, SignatureType> struct type_check;
			//	template<typename U> static yes & test(type_check<typename Sig<U>::fptr, &U::Format>*);
			//	template<typename> static no & test(...);

			//	static bool const value = sizeof(test<T>(0)) == sizeof(yes);
			//};

			typedef std::function<void(std::ostream&, std::string&)> StreamFunction;

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

			// back inline again, RETODO, move out of line if Glib linked in/
			// or not bother as will prob be out of lined by compiler anyway
			// measure impact on multiple copes in translation units?
			inline void AppendFormatHelper(std::ostream & str, const char * format, StreamFunction args[], int argsCount)
			{
					if (!format)
					{
							throw std::logic_error("format cannot be null");
					}

					size_t pos = 0;
					size_t len = ::strlen(format);
					char ch = '\x0';

					for (;;)
					{
							while (pos < len)
							{
									ch = format[pos];
									pos++;
									if (ch == '}')
									{
											if (pos < len && format[pos] == '}') // Treat as escape character for }}
											{
													pos++;
											}
											else
											{
													FormatError();
											}
									}

									if (ch == '{')
									{
											if (pos < len && format[pos] == '{') // Treat as escape character for {{
											{
													pos++;
											}
											else
											{
													pos--;
													break;
											}
									}

									str << ch;
							}

							if (pos == len)
							{
									break;
							}
							pos++;
							if (pos == len || (ch = format[pos]) < '0' || ch > '9')
							{
									FormatError();
							}

							int index = 0;
							do
							{
									index = index * 10 + ch - '0';
									pos++;
									if (pos == len)
									{
											FormatError();
									}
									ch = format[pos];
							} while (ch >= '0' && ch <= '9' && index < 1000000);

							if (index >= argsCount)
							{
									throw std::logic_error("IndexOutOfRange"); // add extra info
							}

							while (pos < len && (ch = format[pos]) == ' ')
							{
									pos++;
							}

							bool leftJustify = false;
							int width = 0;
							if (ch == ',')
							{
									pos++;
									while (pos < len && format[pos] == ' ')
									{
											pos++;
									}

									if (pos == len)
									{
											FormatError();
									}

									ch = format[pos];
									if (ch == '-')
									{
											leftJustify = true;
											pos++;
											if (pos == len)
											{
													FormatError();
											}
											ch = format[pos];
									}

									if (ch < '0' || ch > '9')
									{
											FormatError();
									}

									do
									{
											width = width * 10 + ch - '0';
											pos++;
											if (pos == len)
											{
													FormatError();
											}
											ch = format[pos];
									} while (ch >= '0' && ch <= '9' && width < 1000000);
							}

							while (pos < len && (ch = format[pos]) == ' ')
							{
									pos++;
							}

							const StreamFunction & arg = args[index];
							std::string fmt;

							if (ch == ':')
							{
									std::ostringstream tmp;
									pos++;
									for (;;)
									{
											if (pos == len)
											{
													FormatError();
											}
											ch = format[pos];
											pos++;
											if (ch == '{')
											{
													if (pos < len && format[pos] == '{') // Treat as escape character for {{
													{
															pos++;
													}
													else
													{
															FormatError();
													}
											}
											else if (ch == '}')
											{
													if (pos < len && format[pos] == '}') // Treat as escape character for }}
													{
															pos++;
													}
													else
													{
															pos--;
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
							pos++;

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
					FormatterDetail::StreamFunction ar[]{ ToStreamFunctions(ts)..., nullptr };
					FormatterDetail::AppendFormatHelper(str, format, ar, sizeof...(Ts));
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
			static auto FormatImpl(std::ostream& os, const T& obj, const std::string &fmt, int)
					-> decltype(Policy::Format(os, obj, fmt), void())
			{
					return Policy::Format(os, obj, fmt);
			}

			template <typename T, typename std::enable_if<FormatterDetail::IsFormattable<T>::value>::type* = nullptr>
			static void FormatImpl(std::ostream& stm, const T& value, const std::string &fmt, long)
			{
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
			static void FormatImpl(std::ostream& stm, const T& value, const std::string &fmt, long)
			{
					// handle T==wide string?
					FormatterDetail::CheckEmptyFormat(fmt);
					stm << value;
			}
	};

	typedef FormatterT<FormatterPolicy::Printf> Formatter;
}

#endif // FORMATTER_H
