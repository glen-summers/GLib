#pragma once

#include "GLib/Win/DebugStream.h"
#include "GLib/formatter.h"

namespace GLib
{
	namespace Win
	{
		namespace Debug
		{
			template <typename... Ts>
			void Write(const char * format, Ts&&... ts)
			{
				Formatter::Format(Stream(), format, std::forward<Ts>(ts)...) << std::endl;
			}

			template <typename T>
			void Write(const T & value)
			{
				Stream() << value << std::endl;
			}

			template <>
			inline void Write(const std::wstring & value)
			{
				Stream() << Cvt::w2a(value) << std::endl;
			}
		}
	}
}