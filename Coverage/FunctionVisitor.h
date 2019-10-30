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
		if (propertyName == "name")
		{
			std::ostringstream s;
			if (!function.NameSpace().empty())
			{
				GLib::Xml::Utils::Escape(function.NameSpace(), s) << "::";
			}
			if (!function.ClassName().empty())
			{
				GLib::Xml::Utils::Escape(function.ClassName(), s) << "::";
			}
			GLib::Xml::Utils::Escape(function.FunctionName(), s);
			return f(Value(s.str()));
		}

		if (propertyName == "line")
		{
			unsigned int line = 0;
			constexpr unsigned int offset = 1;
			for (const auto & [file, lines] : function.FileLines())
			{
				if (!lines.empty())
				{
					line = lines.begin()->first - offset;
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
