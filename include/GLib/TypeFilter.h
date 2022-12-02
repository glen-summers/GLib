#pragma once

#include <type_traits>

namespace GLib::Util
{
	template <typename... Types>
	struct Tuple;

	template <>
	struct Tuple<>
	{};

	template <typename First, typename... Rest>
	struct Tuple<First, Rest...> : Tuple<Rest...>
	{
		using Type = Tuple<First, Rest...>;
	};

	template <template <typename> typename Predicate, typename... Types>
	struct TypeFilter;

	template <template <typename> typename Predicate>
	struct TypeFilter<Predicate>
	{
		using TupleType = Tuple<>;
	};

	template <template <typename> typename Predicate, typename First, typename... Rest>
	struct TypeFilter<Predicate, First, Rest...>
	{
		template <typename, typename>
		struct Accumulator;

		template <typename Head, typename... Tail>
		struct Accumulator<Head, Tuple<Tail...>>
		{
			using Types = Tuple<Head, Tail...>;
		};

		using TupleType =
			std::conditional_t<Predicate<First>::value, typename Accumulator<First, typename TypeFilter<Predicate, Rest...>::TupleType>::Types,
												 typename TypeFilter<Predicate, Rest...>::TupleType>;
	};

	template <template <typename, typename...> typename Predicate, typename... Types>
	struct SelfTypeFilter
	{
		template <typename T>
		using AllTypesPredicate = Predicate<T, Types...>;

		using TupleType = typename TypeFilter<AllTypesPredicate, Types...>::TupleType;
	};
}