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

struct Money
{
	long double value;
};

// move to a formatter policy
inline std::ostream & operator << (std::ostream & s, const Money & m)
{
	// stream to wide to correctly convert locale symbols, is there a better better way?
	// maybe when code convert gets fixed
	std::wstringstream wideStream;
	wideStream.imbue(s.getloc());
	wideStream << std::showbase << std::put_money(m.value);
	return s << GLib::Cvt::w2a(wideStream.str());
}

// move to a formatter policy
inline std::ostream & operator << (std::ostream & s, const std::tm & value)
{
	// stream to wide to correctly convert locale symbols, is there a better better way?
	// maybe when code convert gets fixed
	std::wstringstream wideStream;
	wideStream.imbue(s.getloc());

	// %c linux:   Sun 06 Nov 1967 18:00:00 BST
	// %c windows: 06/11/1967 18:00:00
	//wideStream << std::put_time(&value, L"%c");

	wideStream << std::put_time(&value, L"%d/%m/%Y %H:%M:%S");
	return s << GLib::Cvt::w2a(wideStream.str());
}

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