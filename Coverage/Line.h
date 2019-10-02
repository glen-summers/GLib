#pragma once

#include "GLib/Eval/Evaluator.h"

enum class LineCover : int;

struct Line
{
	std::string text;
	std::string number;
	LineCover cover;
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

		if (propertyName == "text")
		{
			return f(Value(line.text));
		}

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};

