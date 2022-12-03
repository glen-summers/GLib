#pragma once

#include <GLib/Cvt.h>
#include <GLib/Win/Transfer.h>

#include <utility>

#include <objbase.h>

namespace GLib::Win
{
	class Bstr
	{
		BSTR value {}; // use unique_ptr?

	public:
		using Base = std::remove_pointer_t<BSTR>;

		Bstr() = default;

		explicit Bstr(Base * const value)
			: value(value)
		{}

		Bstr(Bstr const &) = delete;

		Bstr(Bstr && other) noexcept
			: value {std::exchange(other.value, nullptr)}
		{}

		Bstr & operator=(Bstr const & other) = delete;

		Bstr & operator=(Bstr && other) noexcept
		{
			value = std::exchange(other.value, nullptr);
			return *this;
		}

		~Bstr()
		{
			SysFreeString(value);
		}

		[[nodiscard]] bool HasValue() const
		{
			return value != nullptr;
		}

		[[nodiscard]] std::string Value() const
		{
			return value != nullptr ? Cvt::W2A(value) : std::string {};
		}

		static auto Attach(Base * const value)
		{
			return Bstr(value);
		}

	private:
		Bstr(Base * const other, bool const ignored) noexcept
			: value(other)
		{
			static_cast<void>(ignored);
		}
	};

	inline auto GetAddress(Bstr & value) noexcept
	{
		return Transfer<Bstr, BSTR>(value);
	}
}