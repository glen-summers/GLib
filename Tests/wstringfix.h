#pragma once

#include "GLib/cvt.h"
#include "GLib/scope.h"

#include <iomanip>
#include <utility>

// workaround for strings generating empty test errors with invalid encoding : check result == expected has failed [ != ]
template<typename T> struct StringHolder
{
	typedef std::basic_string<T> StringType;
	StringType value;
	StringHolder(StringType value) : value(std::move(value)) {}
	bool operator==(const StringHolder<T> & right) const
	{
		return right.value == value;
	}
};

template<typename T>
StringHolder<T> Hold(const std::basic_string<T> & value) { return value; }

template<typename T>
std::ostream & operator << (std::ostream & stm, const StringHolder<T> & holder)
{
	std::ios state(nullptr);
	state.copyfmt(stm);
	SCOPE(resetState, [&]() { stm.copyfmt(state); });
	stm << std::hex << std::noshowbase << std::uppercase << std::setfill('0');

	typedef typename std::make_unsigned<T>::type UT;
	for (T c : holder.value)
	{
		stm
			<< "<" << std::left << std::setw(sizeof(c) * 2)
			<< static_cast<uint16_t>(static_cast<UT>(c)) << ">";
	}

	return stm;
}
