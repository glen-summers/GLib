#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>

namespace GLib
{
	template <typename T>
	class Span
	{
	public:
		class Iterator;

		using element_type = T;
		using value_type = std::remove_cv_t<T>;
		using index_type = std::ptrdiff_t;
		using pointer = const element_type *;
		using reference = const element_type &;
		using size_type = index_type;
		using iterator = Iterator;

	private:
		const T * ptr {};
		size_type size {};

	public:
		constexpr Span() noexcept = default;
		constexpr Span(pointer ptr, size_type size)
			: ptr {ptr}
			, size {size}
		{
			if (size < 0)
			{
				throw std::logic_error("size < 0");
			}
		}

		constexpr Iterator begin() const
		{
			return {ptr, size};
		}

		constexpr Iterator end() const
		{
			return {};
		}

		constexpr reference operator[](index_type idx) const
		{
			if (idx >= size || idx < 0)
			{
				throw std::logic_error("IndexOutOfRange");
			}
			return ptr[idx]; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic) todo use c++20 span
		}

		class Iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = typename Span::value_type;
			using difference_type = typename Span::index_type;
			using pointer = const value_type *;
			using reference = const value_type &;

		private:
			pointer ptr = {};
			pointer end = {};

		public:
			constexpr Iterator() noexcept = default;

			constexpr Iterator(pointer ptr, size_type size) noexcept
				: ptr {ptr}
				, end {ptr + size}
			{}

			constexpr bool operator==(const Iterator & other) const
			{
				return ptr == other.ptr;
			}

			constexpr bool operator!=(const Iterator & it) const
			{
				return !(*this == it);
			}

			constexpr Iterator & operator++()
			{
				if (++ptr == end)
				{
					ptr = nullptr;
				}
				return *this;
			}

			constexpr Iterator operator++(int) // NOLINT(cert-dcl21-cpp) until c++/20 span impl
			{
				Iterator it {*this};
				++(*this);
				return it;
			}

			constexpr reference operator*() const
			{
				return *ptr;
			}

			constexpr pointer operator->() const
			{
				return ptr;
			}
		};
	};

	template <typename T>
	Span<T> MakeSpan(const T * value, size_t size)
	{
		return Span<T> {value, static_cast<std::ptrdiff_t>(size)};
	}
}