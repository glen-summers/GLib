#pragma once

#include "Types.h"

#include <regex>
#include <utility>

class Function
{
	inline static std::regex const namespaceRegex{ R"(^(?:[A-Za-z_][A-Za-z_0-9]*::)*)" }; // +some extra unicode chars?

	inline static size_t counter {};

	size_t id = counter++;
	std::string functionName;
	std::string className;
	std::string nameSpace;

	FileLines fileLines;

public:
	Function(std::string name, std::string typeName)
		: functionName(std::move(name))
		, className(std::move(typeName))
	{
		std::smatch m;
		if (!className.empty())
		{
			std::regex_search(className, m, namespaceRegex);
			if (!m.empty())
			{
				size_t len = m[0].str().size();
				if (len >= 2)
				{
					nameSpace = className.substr(0, len - 2);

					if (className.compare(0, nameSpace.size(), nameSpace) == 0)
					{
						className.erase(0, len);
					}
					if (functionName.compare(0, nameSpace.size(), nameSpace) == 0)
					{
						functionName.erase(0, len);
					}
					if (functionName.compare(0, className.size(), className) == 0)
					{
						functionName.erase(0, className.size() + 2);
					}
					RemoveTemplateDefinition(className);
					RemoveTemplateDefinition(functionName);
				}
			}
		}
		else
		{
			std::regex_search(functionName, m, namespaceRegex);
			if (!m.empty())
			{
				size_t len = m[0].str().size();
				if (len >= 2)
				{
					nameSpace = functionName.substr(0, len - 2);
					functionName.erase(0, len);
				}
				RemoveTemplateDefinition(functionName);
			}
		}
	}

	size_t Id() const { return id; }
	std::string Namespace() const { return nameSpace; }
	std::string ClassName() const { return className; }
	std::string FunctionName() const { return functionName; }

	const FileLines & FileLines() const
	{
		return fileLines;
	}

	std::string FullName() const
	{
		return className.empty()
			? nameSpace + "::" + functionName
			: nameSpace + "::" + className + "::" + functionName;
	}

	// called when another address seen for the same function symbolId
	void Accumulate(const Address & address)
	{
		bool const visited = address.Visited();
		for (const auto & fileLineIt : address.FileLines())
		{
			const std::map<unsigned int, bool> & lines = fileLineIt.second;
			std::map<unsigned, bool> & pairs = fileLines[fileLineIt.first];

			for (const auto & lineIt : lines) // map merge method?
			{
				pairs[lineIt.first] |= visited;
			}
		}
	}

	// called for e.g. class templates, merge with above
	void Merge(const Function & other)
	{
		for (const auto & fileLineIt : other.FileLines())
		{
			const std::map<unsigned int, bool> & lines = fileLineIt.second;
			std::map<unsigned, bool> & pairs = fileLines[fileLineIt.first];

			for (const auto & lineIt : lines) // map merge method?
			{
				pairs[lineIt.first] |= lineIt.second;
			}
		}
	}

	size_t CoveredLines() const
	{
		size_t total{};
		for (const auto & x : fileLines)
		{
			for (const auto & y : x.second)
			{
				// improve? keep tally?
				if (y.second)
				{
					++total;
				}
			}
		}
		return total;
	}

	size_t AllLines() const
	{
		size_t total{};
		for (const auto & x : fileLines)
		{
			total += x.second.size();
		}
		return total;
	}

private:
	static void RemoveTemplateDefinition(std::string & name)
	{
		const auto bra = name.find('<');
		// remove Class[<...>],  ignore functionName like "<lambda_94309dc8705386c058f3254e10a42590>"
		if (bra != 0 && bra != std::string::npos)
		{
			const auto ket = name.rfind('>');
			if (ket > bra)
			{
				name.erase(bra, ket - bra + 1);
			}
		}
	}
};
