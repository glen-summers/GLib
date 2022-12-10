#pragma once

#include <GLib/Eval/Evaluator.h>

enum class LineCover : uint8_t;

struct Line
{
	std::string Text;
	unsigned int Number;
	std::string PaddedNumber;
	LineCover Cover;
	bool HasLink;
};

template <>
struct GLib::Eval::Visitor<Line>
{
	static void Visit(Line const & line, std::string const & propertyName, ValueVisitor const & visitor)
	{
		if (propertyName == "cover")
		{
			return visitor(Value(line.Cover));
		}

		if (propertyName == "number")
		{
			return visitor(Value(line.Number));
		}

		if (propertyName == "paddedNumber")
		{
			return visitor(Value(line.PaddedNumber));
		}

		if (propertyName == "text")
		{
			return visitor(Value(line.Text));
		}

		if (propertyName == "hasLink")
		{
			return visitor(Value(line.HasLink));
		}

		if (propertyName == "hasNoLink") // todo !${value}
		{
			return visitor(Value(!line.HasLink));
		}

		throw std::runtime_error("Unknown property : '" + propertyName + '\'');
	}
};
