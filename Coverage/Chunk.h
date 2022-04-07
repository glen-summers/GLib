#pragma once

#include "LineCover.h"

#include <GLib/Eval/Value.h>

struct Chunk
{
	LineCover Cover;
	float Size;
};

template <>
struct GLib::Eval::Visitor<Chunk>
{
	static void Visit(const Chunk & chunk, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "cover")
		{
			return f(Value(chunk.Cover));
		}

		if (propertyName == "size")
		{
			return f(Value(chunk.Size));
		}

		throw std::runtime_error("Unknown property : '" + propertyName + '\'');
	}
};
