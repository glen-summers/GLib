#pragma once

#include "LineCover.h"

#include "GLib/Eval/Value.h"

struct Chunk
{
	LineCover cover;
	float size;
};

template <>
struct GLib::Eval::Visitor<Chunk>
{
	static void Visit(const Chunk & chunk, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "cover")
		{
			return f(Value(chunk.cover));
		}

		if (propertyName == "size")
		{
			return f(Value(chunk.size));
		}

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};



