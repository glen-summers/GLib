#pragma once

#include <GLib/Formatter.h>
#include <GLib/Win/DebugStream.h>

namespace GLib::Win::Debug
{
	template <typename... Ts>
	void Write(std::string_view format, Ts &&... ts)
	{
		Formatter::Format(Stream(), format, std::forward<Ts>(ts)...) << std::endl;
	}

	template <typename T>
	void Write(const T & value)
	{
		Stream() << value << std::endl;
	}

	template <typename T>
	void Write(const T * value)
	{
		Stream() << value << std::endl;
	}

	inline void Write(std::string_view value)
	{
		Stream() << value << std::endl;
	}

	inline void Write(std::wstring_view value)
	{
		Stream() << Cvt::W2A(value) << std::endl;
	}
}