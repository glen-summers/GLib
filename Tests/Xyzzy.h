#pragma once

#include <ostream>
#include <string>
#include <iomanip>

#include "GLib/cvt.h"

struct Xyzzy
{
	void Format(std::ostream & s, const std::string & fmt) const
	{
		s << fmt << ":plover";
	}
};

std::ostream & operator << (std::ostream & s, const Xyzzy &)
{
	return s << "plover";
};

struct Xyzzy2
{};

std::ostream & operator << (std::ostream & s, const Xyzzy2 &)
{
	return s << "plover2";
};

struct CopyCheck
{
	mutable int copies = 0;
	int moves = 0;

	CopyCheck() = default;

	CopyCheck(const CopyCheck& other)
		: copies(++other.copies)
		, moves(other.moves)
	{}

	CopyCheck& operator=(const CopyCheck& other)
	{
		if (this != &other)
		{
			copies = ++other.copies;
		}
		return *this;
	}

	CopyCheck(CopyCheck&& other)
		: copies(other.copies)
		, moves(++other.moves)
	{}
};

inline std::ostream & operator<<(std::ostream & s, const CopyCheck & c)
{
	return s << c.copies << ":" << c.moves;
}