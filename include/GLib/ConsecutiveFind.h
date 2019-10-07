#pragma once

namespace GLib::Util
{
	template<typename InputIt, typename T>
	constexpr InputIt FindNot(InputIt first, InputIt last, const T & value)
	{
		while (first != last && *first == value) ++first;
		return first;
	}

	template<typename InputIt, typename T, typename BinaryPredicate>
	constexpr InputIt FindNextIf(InputIt first, InputIt last, const T & value, BinaryPredicate predicate)
	{
		while (first != last && !predicate(*first , value)) ++first;
		return first;
	}

	template <typename Iterator>
	Iterator ConsecutiveFind(Iterator first, Iterator last)
	{
		return FindNot(std::next(first), last, *first);
	}

	template <typename Iterator, typename Pred>
	Iterator ConsecutiveFind(Iterator first, Iterator end, Pred pred)
	{
		return FindNextIf(std::next(first), end, *first, pred);
	}
}