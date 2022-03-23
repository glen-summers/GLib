#pragma once

#include <ostream>
#include <string>

struct Xyzzy
{
	void Format(std::ostream & s, const std::string & fmt) const
	{
		s << fmt << ":plover";
	}
};

std::ostream & operator<<(std::ostream & s, const Xyzzy &)
{
	return s << "plover";
}

struct Xyzzy2
{};

std::ostream & operator<<(std::ostream & s, const Xyzzy2 &)
{
	return s << "plover2";
}

struct CopyCheck
{
	mutable int Copies = 0;
	int Moves = 0;

	CopyCheck() = default;

	CopyCheck(const CopyCheck & other)
		: Copies(++other.Copies)
		, Moves(other.Moves)
	{}

	CopyCheck & operator=(const CopyCheck & other)
	{
		if (this != &other)
		{
			Copies = ++other.Copies;
		}
		return *this;
	}

	CopyCheck(CopyCheck && other) noexcept
		: Copies(other.Copies)
		, Moves(++other.Moves)
	{}
};

inline std::ostream & operator<<(std::ostream & s, const CopyCheck & c)
{
	return s << c.Copies << ":" << c.Moves;
}