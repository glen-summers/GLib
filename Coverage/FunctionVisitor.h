#pragma once

#include "FunctionCoverage.h"
#include "LineCover.h"

#include <GLib/Eval/Value.h>
#include <GLib/Xml/Utils.h>

template <>
struct GLib::Eval::Visitor<FunctionCoverage>
{
	static void Visit(const FunctionCoverage & fc, const std::string & propertyName, const ValueVisitor & f)
	{
		if (propertyName == "name")
		{
			std::ostringstream s;
			if (!fc.NameSpace().empty())
			{
				GLib::Xml::Utils::Escape(fc.NameSpace(), s) << "::";
			}
			if (!fc.ClassName().empty())
			{
				GLib::Xml::Utils::Escape(fc.ClassName(), s) << "::";
			}
			GLib::Xml::Utils::Escape(fc.FunctionName(), s);
			return f(Value(s.str()));
		}
		if (propertyName == "line")
		{
			return f(Value(fc.Line()));
		}
		if (propertyName == "coveredLines")
		{
			return f(Value(fc.CoveredLines()));
		}
		if (propertyName == "coverableLines")
		{
			return f(Value(fc.CoverableLines()));
		}

		if (propertyName == "cover")
		{
			return f(Value(fc.CoveredLines() != 0 ? LineCover::Covered : LineCover::NotCovered));
		}

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};
