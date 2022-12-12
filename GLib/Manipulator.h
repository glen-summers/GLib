#pragma once

#include <ostream>

namespace Detail
{
	template <typename Argument>
	class Manipulator
	{
		std::ostream & (*function)(std::ostream &, Argument);
		Argument argument;

	public:
		Manipulator(std::ostream & (*function)(std::ostream &, Argument), Argument const argument)
			: function {function}
			, argument {argument}
		{}

		std::ostream & Manipulate(std::ostream & stm) const
		{
			return function(stm, argument);
		}
	};

	template <typename Argument>
	std::ostream & operator<<(std::ostream & stream, Manipulator<Argument> const & manipulator)
	{
		return manipulator.Manipulate(stream);
	}
}

template <typename Function, typename Argument>
auto Manipulate(Function const function, Argument const argument)
{
	return Detail::Manipulator<Argument> {function, argument};
}