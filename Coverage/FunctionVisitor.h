#pragma once

#include "GLib/Eval/Value.h"
#include "GLib/Xml/Utils.h"

#include "Function.h"
#include "LineCover.h"

template <>
struct GLib::Eval::Visitor<Function>
{
	static void Visit(const Function & function, const std::string & propertyName, const ValueVisitor & f)
	{
		constexpr char separator[] = "::";
		if (propertyName == "Name")
		{
			std::ostringstream s;
			if (!function.NameSpace().empty())
			{
				GLib::Xml::Utils::Escape(function.NameSpace(), s) << separator;
			}
			if (!function.ClassName().empty())
			{
				GLib::Xml::Utils::Escape(function.ClassName(), s) << separator;
			}
			GLib::Xml::Utils::Escape(function.FunctionName(), s);
			return f(Value(s.str()));
		}

		if (propertyName == "cover")
		{
			// improve
			return f(Value(function.CoveredLines() !=0 ? LineCover::Covered : LineCover::NotCovered));
		}
	}
};
