#pragma once

#include <chrono>
#include <iomanip>
#include <ostream>

inline std::ostream & operator<< (std::ostream & s, std::chrono::nanoseconds duration)
{
	if (std::chrono::duration_cast<std::chrono::seconds>(duration).count() == 0)
	{
		constexpr auto milli = 1000;
		return s << std::setprecision(1) << std::fixed << std::chrono::duration<double>(duration).count() * milli << "ms";
	}

	constexpr auto HoursInAnEarthDay = 24; // until c++20 days constant
	using days = std::chrono::duration<long, std::ratio_multiply<std::chrono::hours::period, std::ratio<HoursInAnEarthDay>>>;
	auto day = std::chrono::duration_cast<days>(duration);
	duration -= day;
	auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;
	auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;

	constexpr auto NanoSecondsInAMillisecond = 1000000;
	long long millis = duration.count() / NanoSecondsInAMillisecond;
	int effectiveDigits = 3;
	while (effectiveDigits > 0)
	{
		constexpr auto DecimalModulus = 10;
		if (millis % DecimalModulus == 0)
		{
			millis /= DecimalModulus;
			effectiveDigits--;
		}
		else
		{
			break;
		}
	}

	if (day.count() != 0)
	{
		s << day.count() << ".";
	}
	s << hours.count() << ":";
	s << minutes.count() << ":";

	const auto Nanos= 1e9;
	return s << std::setprecision(effectiveDigits) << std::fixed << duration.count() / Nanos;
}