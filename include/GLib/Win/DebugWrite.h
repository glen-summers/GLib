#pragma once

#include <GLib/Formatter.h>
#include <GLib/Win/DebugStream.h>

namespace GLib::Win::Debug
{
	template <typename... Ts>
	void Write(std::string_view format, Ts &&... values)
	{
		Formatter::Format(Stream(), format, std::forward<Ts>(values)...) << std::endl;
	}

	template <typename T>
	void Write(T const & value)
	{
		Stream() << value << std::endl;
	}

	template <typename T>
	void Write(T const * value)
	{
		Stream() << value << std::endl;
	}

	inline void Write(std::string_view const value)
	{
		Stream() << value << std::endl;
	}

	inline void Write(std::wstring_view const value)
	{
		Stream() << Cvt::W2A(value) << std::endl;
	}
}