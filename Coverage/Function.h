#pragma once

#include "Types.h"

#include <regex>

struct LinesCoverage
{
	size_t linesCovered{}, linesNotCovered{};
};

class Function
{
	inline static std::regex const namespaceRegex{ R"(^(?:[A-Za-z_][A-Za-z_0-9]*::)*)" }; // +some extra unicode chars?

	inline static size_t counter {};

	size_t id = counter++;
	std::string functionName;
	std::string className;
	std::string nameSpace;
	LinesCoverage coverage;
	FileLines fileLines;

public:
	Function(const std::string & name, const std::string & typeName)
		: functionName(name)
		, className(typeName)
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

	const LinesCoverage & Coverage() const
	{
		return coverage;
	}

	const FileLines& FileLines() const
	{
		return fileLines;
	}

	std::string FullName() const
	{
		return className.empty()
			? nameSpace + "::" + functionName
			: nameSpace + "::" + className + "::" + functionName;
	}

	void Accumulate(const Address & address, LinesCoverage & allCoverage)
	{
		for (const auto & fileLineIt : address.FileLines())
		{
			const auto & lines = fileLineIt.second;

			const size_t lineCount = lines.size();
			if (address.Visited())
			{
				allCoverage.linesCovered += lineCount;
				coverage.linesCovered += lineCount;

				auto copy = lines;
				fileLines[fileLineIt.first].merge(copy);
			}
			else
			{
				allCoverage.linesNotCovered += lineCount;
				coverage.linesNotCovered += lineCount;
			}
		}
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
