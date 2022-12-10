#pragma once

#include <ostream>

enum class CoverageLevel : uint8_t
{
	Red,
	Amber,
	Green
};

constexpr unsigned int HundredPercent = 100;

inline unsigned int Percentage(size_t const value, size_t const amount)
{
	return static_cast<unsigned int>(HundredPercent * value / amount);
}

inline std::ostream & operator<<(std::ostream & stm, CoverageLevel const coverageLevel)
{
	switch (coverageLevel)
	{
		case CoverageLevel::Red:
			stm << "red";
			break;

		case CoverageLevel::Amber:
			stm << "amber";
			break;

		case CoverageLevel::Green:
			stm << "green";
			break;
	}
	return stm;
}

inline CoverageLevel GetCoverageLevel(unsigned int const coveragePercent)
{
	constexpr uint8_t lowValue = 70; // config
	constexpr uint8_t highValue = 90;

	return coveragePercent < lowValue ? CoverageLevel::Red : coveragePercent < highValue ? CoverageLevel::Amber : CoverageLevel::Green;
}
