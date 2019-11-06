#pragma once

#include "GLib/cvt.h"
#include "GLib/Win/Transfer.h"

#include <utility>

#include <objbase.h>

namespace GLib::Win
{
	class Bstr
	{
		BSTR value{}; // use unique_ptr?

		Bstr(BSTR value) : value(value)
		{}

	public:
		Bstr() = default;

		Bstr(const Bstr &) = delete;

		Bstr(Bstr && other) noexcept : value{std::exchange(other.value, nullptr)}
		{}

		Bstr & operator = (const Bstr & other) noexcept = delete;

		Bstr & operator = (Bstr && other) noexcept
		{
			value = std::exchange(other.value, nullptr);
			return *this;
		}

		~Bstr()
		{
			::SysFreeString(value);
		}

		std::string Value() const
		{
			return value != nullptr ? Cvt::w2a(value) : "";
		}

		static Bstr Attach(BSTR value)
		{
			return {value};
		}

	private:
		Bstr(BSTR other, bool ignored) noexcept
			: value(other)
		{
			(void)ignored;
		}
	};

	auto GetAddress(Bstr & value) noexcept
	{
		return Transfer<Bstr, BSTR>(value);
	}
}