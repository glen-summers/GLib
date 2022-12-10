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
	static void Visit(Chunk const & chunk, std::string const & propertyName, ValueVisitor const & visitor)
	{
		if (propertyName == "cover")
		{
			return visitor(Value(chunk.Cover));
		}

		if (propertyName == "size")
		{
			return visitor(Value(chunk.Size));
		}

		throw std::runtime_error("Unknown property : '" + propertyName + '\'');
	}
};
