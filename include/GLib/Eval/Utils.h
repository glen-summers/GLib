#pragma once

#include <GLib/Compat.h>

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
			static auto test(int) -> decltype(std::to_string(C {}), std::true_type());

			template <typename>
			static auto test(...) -> decltype(std::false_type());

			static constexpr bool value = decltype(test<T>(0))::value;
		};

		template <typename T>
		struct CanStream
		{
			template <typename C>
			static auto test(int) -> decltype(std::declval<std::ostream &>() << std::declval<C>(), std::true_type());

			template <typename>
			static auto test(...) -> decltype(std::false_type());

			static constexpr bool value = decltype(test<T>(0))::value;
		};

		// basic container test has iterator, add begin\end.
		template <typename T>
		struct IsContainer
		{
			template <typename C>
			static auto test(int) -> decltype(typename C::const_iterator {}, std::true_type());

			template <typename>
			static auto test(...) -> decltype(std::false_type());

			static constexpr bool value = decltype(test<T>(0))::value;
		};
	}

	inline std::string ToString(bool const & value)
	{
		return value ? "true" : "false";
	}

	template <typename T, std::enable_if_t<Detail::HasToString<T>::value> * = nullptr>
	std::string ToString(T const & value)
	{
		return std::to_string(value);
	}

	template <typename T, std::enable_if_t<!Detail::HasToString<T>::value && Detail::CanStream<T>::value> * = nullptr>
	std::string ToString(T const & value)
	{
		std::ostringstream stm;
		stm << value;
		return stm.str();
	}

	template <typename T, std::enable_if_t<!Detail::HasToString<T>::value && !Detail::CanStream<T>::value> * = nullptr>
	std::string ToString(T const & value)
	{
		static_cast<void>(value);
		throw std::runtime_error(std::string("Cannot convert type to string : ") + Compat::Unmangle(typeid(T).name()));
	}

	inline std::string ToString(std::string const & value)
	{
		return value;
	}
}
