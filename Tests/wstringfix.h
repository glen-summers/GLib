#pragma once

#include "GLib/cvt.h"
#include "GLib/scope.h"

#include <iomanip>

// workaround wstring generates empty test errors: 
// convertertests.cpp(40): error: in "ConverterTests/Test1": check result == expected has failed [ != ]
template<typename T> struct StringHolder
{
	typedef std::basic_string<T> StringType;
	StringType value;
	StringHolder(const StringType & value) : value(value) {}
	bool operator==(const StringHolder<T> & right) const {
		return right.value == value;
	}
};

template<typename T>
StringHolder<T> Hold(const std::basic_string<T> & value) {
	return value;
}

template<typename T>
std::ostream & operator << (std::ostream & stm, const StringHolder<T> & holder)
{
	std::ios state(nullptr);
	state.copyfmt(stm);
	SCOPE(resetState, [&]() { stm.copyfmt(state); });
	stm << std::hex << std::noshowbase << std::setfill('0');

	for (T c : holder.value)
	{
		stm << std::left << std::setw(sizeof(c) * 2) << c << " ";
	}
	stm.copyfmt(state);
	return stm;
}
