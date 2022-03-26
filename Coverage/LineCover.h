#pragma once

#include <array>
#include <ostream>

enum class LineCover : int
{
	None,
	Covered,
	NotCovered
};

inline std::ostream & operator<<(std::ostream & s, const LineCover & cov)
{
	constexpr auto a = std::array {std::string_view {}, std::string_view {"cov"}, std::string_view {"ncov"}};
	return s << a.at(static_cast<int>(cov));
}
