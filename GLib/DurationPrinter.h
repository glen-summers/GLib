#pragma once

#include <chrono>
#include <iomanip>
#include <ostream>

inline std::ostream & operator<<(std::ostream & s, std::chrono::nanoseconds duration)
{
	if (std::chrono::duration_cast<std::chrono::seconds>(duration).count() == 0)
	{
		constexpr auto toMilliseconds = 1000;
		return s << std::setprecision(1) << std::fixed << std::chrono::duration<double>(duration).count() * toMilliseconds << "ms";
	}

	auto const day = std::chrono::duration_cast<std::chrono::days>(duration);
	duration -= day;
	auto const hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;
	auto const minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;
	auto const seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

	int effectiveDigits = 3;
	while (effectiveDigits > 0)
	{
		constexpr auto DecimalModulus = 10;
		if (milliseconds % DecimalModulus == 0)
		{
			milliseconds /= DecimalModulus;
			effectiveDigits--;
		}
		else
		{
			break;
		}
	}

	if (day.count() != 0)
	{
		s << day.count() << '.';
	}

	s << hours.count() << ':' << minutes.count() << ':' << seconds.count();

	if (milliseconds != 0)
	{
		s << '.' << std::setprecision(effectiveDigits) << std::fixed << milliseconds;
	}

	return s;
}