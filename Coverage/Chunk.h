#pragma once

#include "LineCover.h"

#include <GLib/Eval/Value.h>

struct Chunk
{
	LineCover const Cover;
	float Size;
};

template <>
struct GLib::Eval::Visitor<Chunk>
{
	static void Visit(Chunk const & chunk, std::string const & propertyName, ValueVisitor const & f)
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
