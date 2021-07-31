#pragma once

#include <ostream>

enum class CoverageLevel : int
{
	Red,
	Amber,
	Green
};

constexpr unsigned int HundredPercent = 100;

inline unsigned int Percentage(size_t value, size_t amount)
{
	return static_cast<unsigned int>(HundredPercent * value / amount);
}

inline std::ostream & operator<<(std::ostream & s, enum CoverageLevel coverageLevel)
{
	switch (coverageLevel)
	{
		case CoverageLevel::Red:
			s << "red";
			break;

		case CoverageLevel::Amber:
			s << "amber";
			break;

		case CoverageLevel::Green:
			s << "green";
			break;

		default:;
	}
	return s;
}

inline enum CoverageLevel CoverageLevel(unsigned int coveragePercent)
{
	const int lowValue = 70; // config
	const int highValue = 90;

	return coveragePercent < lowValue ? CoverageLevel::Red : coveragePercent < highValue ? CoverageLevel::Amber : CoverageLevel::Green;
}
