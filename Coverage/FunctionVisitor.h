#pragma once

#include "FunctionCoverage.h"
#include "LineCover.h"

#include <GLib/Eval/Value.h>
#include <GLib/Xml/Utils.h>

template <>
struct GLib::Eval::Visitor<FunctionCoverage>
{
	static void Visit(FunctionCoverage const & coverage, std::string const & propertyName, ValueVisitor const & visitor)
	{
		if (propertyName == "name")
		{
			std::ostringstream stm;
			if (!coverage.NameSpace().empty())
			{
				Xml::Utils::Escape(coverage.NameSpace(), stm) << "::";
			}
			if (!coverage.ClassName().empty())
			{
				Xml::Utils::Escape(coverage.ClassName(), stm) << "::";
			}
			Xml::Utils::Escape(coverage.FunctionName(), stm);
			return visitor(Value(stm.str()));
		}
		if (propertyName == "line")
		{
			return visitor(Value(coverage.Line()));
		}
		if (propertyName == "coveredLines")
		{
			return visitor(Value(coverage.CoveredLines()));
		}
		if (propertyName == "coverableLines")
		{
			return visitor(Value(coverage.CoverableLines()));
		}

		if (propertyName == "cover")
		{
			return visitor(Value(coverage.CoveredLines() != 0 ? LineCover::Covered : LineCover::NotCovered));
		}

		throw std::runtime_error("Unknown property : '" + propertyName + '\'');
	}
};
