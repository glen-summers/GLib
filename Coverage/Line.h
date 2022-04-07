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
	static void Visit(const Line & line, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "cover")
		{
			return f(Value(line.Cover));
		}

		if (propertyName == "number")
		{
			return f(Value(line.Number));
		}

		if (propertyName == "paddedNumber")
		{
			return f(Value(line.PaddedNumber));
		}

		if (propertyName == "text")
		{
			return f(Value(line.Text));
		}

		if (propertyName == "hasLink")
		{
			return f(Value(line.HasLink));
		}

		if (propertyName == "hasNoLink") // todo !${value}
		{
			return f(Value(!line.HasLink));
		}

		throw std::runtime_error("Unknown property : '" + propertyName + '\'');
	}
};
