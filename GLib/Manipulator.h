#pragma once

#include <ostream>

namespace Detail
{
	template <typename Argument>
	class Manipulator
	{
		std::ostream &(* function)(std::ostream &, Argument);
		Argument argument;

	public:
		Manipulator(std::ostream &(* function)(std::ostream &, Argument), Argument argument)
			: function{function}
			, argument{argument}
		{}

		std::ostream & Manipulate(std::ostream & s) const
		{
			return function(s, argument);
		}
	};

	template <typename Argument>
	std::ostream & operator << (std::ostream & stream, const Manipulator<Argument> & manipulator)
	{
		return manipulator.Manipulate(stream);
	}
}

template <typename Function, typename Argument>
auto Manipulate(Function function, Argument argument)
{
	return Detail::Manipulator<Argument>{function, argument};
}