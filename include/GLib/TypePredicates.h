#pragma once

#include <GLib/TypeFilter.h>

namespace GLib::TypePredicates
{
	template <typename T1, typename T2>
	struct IsBaseOf : std::bool_constant<std::is_base_of_v<T1, T2> && !std::is_same_v<T1, T2>>
	{};

	template <typename T1, typename T2>
	using IsDerivedFrom = IsBaseOf<T2, T1>;

	template <typename T, typename... Types>
	struct IsBaseOfAny;

	template <typename T>
	struct IsBaseOfAny<T> : std::bool_constant<false>
	{};

	template <typename T, typename First, typename... Rest>
	struct IsBaseOfAny<T, First, Rest...> : std::bool_constant<IsBaseOf<T, First>::value || IsBaseOfAny<T, Rest...>::value>
	{};

	template <typename T, typename... Types>
	struct HasNoInheritor;

	template <typename T>
	struct HasNoInheritor<T> : std::bool_constant<false>
	{};

	template <typename T, typename First, typename... Rest>
	struct HasNoInheritor<T, First, Rest...> : std::bool_constant<!IsBaseOf<T, First>::value && !IsBaseOfAny<T, Rest...>::value>
	{};
}