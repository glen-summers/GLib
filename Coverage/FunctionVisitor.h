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
		if (propertyName == "name")
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

		if (propertyName == "line")
		{
			int line = -1;
			for (const auto & [file, lines] : function.FileLines())
			{
				if (!lines.empty())
				{
					line = lines.begin()->first - 1; // -1 to include function defn
					break;
				}
			}

			return f(Value(line));
		}

		if (propertyName == "cover")
		{
			// improve
			return f(Value(function.CoveredLines() !=0 ? LineCover::Covered : LineCover::NotCovered));
		}

		throw std::runtime_error(std::string("Unknown property : '") + propertyName + '\'');
	}
};
