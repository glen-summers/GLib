#pragma once

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace GLib::Util
{
	namespace Detail
	{
		struct OverflowPolicy
		{
			// use std::overflow\underflow?
			static void Underflow(bool err)
			{
				if (err)
				{
					throw std::runtime_error("Underflow");
				}
			}

			static void Overflow(bool err)
			{
				if (err)
				{
					throw std::runtime_error("Overflow");
				}
			}
		};

		template <typename T, typename S, typename OverflowPolicy>
		struct OverflowHandler
		{
			using Target = T;
			using Source = S;

			static void UnderflowMin(Source source)
			{
				OverflowPolicy::Underflow(source < static_cast<Source>(std::numeric_limits<Target>::lowest()));
			}

			static void UnderflowZero(Source source)
			{
				OverflowPolicy::Underflow(source < static_cast<Source>(0));
			}

			static void OverflowMax(Source source)
			{
				OverflowPolicy::Overflow(source > static_cast<Source>(std::numeric_limits<Target>::max()));
			}
		};

		template <typename T, typename S>
		struct Traits
		{
			using Target = T;
			using Source = S;
			using Overflow = OverflowHandler<T, S, OverflowPolicy>;

			static Target Convert(Source source)
			{
				return static_cast<Target>(source);
			}
		};

		template <typename Target, typename Source>
		struct Flags
		{
			static constexpr bool SameSign = std::is_signed<Target>::value == std::is_signed<Source>::value;
			static constexpr bool SignedToUnsigned = std::is_signed<Source>::value && std::is_unsigned<Target>::value;
			static constexpr bool UnsignedToSigned = std::is_unsigned<Source>::value && std::is_signed<Target>::value;
			static constexpr bool TargetSmaller = std::numeric_limits<Target>::digits < std::numeric_limits<Source>::digits;

			static constexpr bool TrivialCopy = !SignedToUnsigned && !TargetSmaller;
			static constexpr bool CheckMinMax = SameSign && TargetSmaller;
			static constexpr bool CheckZeroMax = SignedToUnsigned && TargetSmaller;
			static constexpr bool CheckZero = SignedToUnsigned && !TargetSmaller;
			static constexpr bool CheckMax = UnsignedToSigned && TargetSmaller;
		};

		template <typename T, typename S, typename Traits, typename enabled = void>
		struct RangeChecker
		{};

		template <typename T, typename S, typename Traits>
		struct RangeChecker<T, S, Traits, typename std::enable_if<Flags<T, S>::TrivialCopy>::type>
		{
			static T Check(S source)
			{
				return source;
			}
		};

		template <typename T, typename S, typename Traits>
		struct RangeChecker<T, S, Traits, typename std::enable_if<Flags<T, S>::CheckMinMax>::type>
		{
			static T Check(S source)
			{
				Traits::Overflow::UnderflowMin(source);
				Traits::Overflow::OverflowMax(source);
				return Traits::Convert(source);
			}
		};

		template <typename T, typename S, typename Traits>
		struct RangeChecker<T, S, Traits, typename std::enable_if<Flags<T, S>::CheckZeroMax>::type>
		{
			static T Check(S source)
			{
				Traits::Overflow::UnderflowZero(source);
				Traits::Overflow::OverflowMax(source);
				return Traits::Convert(source);
			}
		};

		template <typename T, typename S, typename Traits>
		struct RangeChecker<T, S, Traits, typename std::enable_if<Flags<T, S>::CheckZero>::type>
		{
			static T Check(S source)
			{
				Traits::Overflow::UnderflowZero(source);
				return Traits::Convert(source);
			}
		};

		template <typename T, typename S, typename Traits>
		struct RangeChecker<T, S, Traits, typename std::enable_if<Flags<T, S>::CheckMax>::type>
		{
			static T Check(S source)
			{
				Traits::Overflow::OverflowMax(source);
				return Traits::Convert(source);
			}
		};
	}

	template <typename Target, typename Source>
	Target checked_cast(Source source)
	{
		using Traits = Detail::Traits<Target, Source>;
		return Detail::RangeChecker<typename Traits::Target, typename Traits::Source, Traits>::Check(source);
	}
}