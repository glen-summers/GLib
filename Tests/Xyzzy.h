#pragma once

#include <ostream>
#include <string>

struct Xyzzy
{
	static void Format(std::ostream & s, const std::string & fmt)
	{
		s << fmt << ":plover";
	}
};

inline std::ostream & operator<<(std::ostream & s, const Xyzzy & /*unused*/)
{
	return s << "plover";
}

struct Xyzzy2
{};

inline std::ostream & operator<<(std::ostream & s, const Xyzzy2 & /*unused*/)
{
	return s << "plover2";
}

class CopyCheck
{
	mutable int copies = 0;
	int moves = 0;

public:
	CopyCheck() = default;

	CopyCheck(const CopyCheck & other)
		: copies {++other.copies}
		, moves {other.moves}
	{}

	CopyCheck(CopyCheck && other) noexcept
		: copies {other.copies}
		, moves {++other.moves}
	{}

	~CopyCheck() = default;

	CopyCheck & operator=(const CopyCheck & other)
	{
		if (this != &other)
		{
			copies = ++other.copies;
		}
		return *this;
	}

	CopyCheck & operator=(CopyCheck && other) = default;

	int Copies() const
	{
		return copies;
	}

	int Moves() const
	{
		return moves;
	}
};

inline std::ostream & operator<<(std::ostream & s, const CopyCheck & c)
{
	return s << c.Copies() << ":" << c.Moves();
}