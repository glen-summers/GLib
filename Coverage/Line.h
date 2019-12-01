#pragma once

#include <GLib/Eval/Evaluator.h>

enum class LineCover : int;

struct Line
{
	std::string text;
	unsigned int number;
	std::string paddedNumber;
	LineCover cover;
	bool hasLink;
};

template <>
struct GLib::Eval::Visitor<Line>
{
	static void Visit(const Line & line, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "cover")
		{
			return f(Value(line.cover));
		}

		if (propertyName == "number")
		{
			return f(Value(line.number));
		}

		if (propertyName == "paddedNumber")
		{
			return f(Value(line.paddedNumber));
		}

		if (propertyName == "text")
		{
			return f(Value(line.text));
		}

		if (propertyName == "hasLink")
		{
			return f(Value(line.hasLink));
		}

		if (propertyName == "hasNoLink") // todo !${value}
		{
			return f(Value(!line.hasLink));
		}

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};

