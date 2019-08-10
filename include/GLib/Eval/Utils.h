#pragma once

#include "GLib/compat.h"

#include <sstream>
#include <string>
#include <type_traits>

namespace GLib::Eval::Utils
{
	namespace Detail
	{
		template <typename T>
		struct HasToString
		{
			template <typename C>
			static auto test(int) -> decltype(std::to_string(C{}), std::true_type());

			template <typename>
			static auto test(...) -> decltype(std::false_type());

			static constexpr bool value = decltype(test<T>(0))::value;
		};

		template<typename T>
		struct CanStream
		{
			template <typename C>
			static auto test(int) -> decltype(std::declval<std::ostream&>() << std::declval<C>(), std::true_type());

			template <typename>
			static auto test(...) -> decltype(std::false_type());

			static constexpr bool value = decltype(test<T>(0))::value;
		};

		// basic container test has iterator, add begin\end.
		template<typename T>
		struct IsContainer
		{
			template <typename C>
			static auto test(int) -> decltype(typename C::const_iterator{}, std::true_type());

			template <typename>
			static auto test(...) -> decltype(std::false_type());

			static constexpr bool value = decltype(test<T>(0))::value;
		};
	}

	template <typename T, std::enable_if_t<Detail::HasToString<T>::value>* = nullptr>
	std::string ToString(const T & value)
	{
		return std::to_string(value);
	}

	template <typename T, std::enable_if_t<!Detail::HasToString<T>::value && Detail::CanStream<T>::value>* = nullptr>
	std::string ToString(const T & value)
	{
		std::ostringstream s;
		s << value;
		return s.str();
	}

	template <typename T, std::enable_if_t<!Detail::HasToString<T>::value && !Detail::CanStream<T>::value>* = nullptr>
	std::string ToString(const T & value)
	{
		(void)value;
		throw std::runtime_error(std::string("Cannot convert type to string : ") + Compat::Unmangle(typeid(T).name()));
	}

	inline std::string ToString(const std::string & value)
	{
		return value;
	}
}
