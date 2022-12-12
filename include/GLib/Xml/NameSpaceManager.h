#pragma once

#include <stack>
#include <stdexcept>
#include <unordered_map>

namespace GLib::Xml
{
	class NameSpaceManager
	{
		static constexpr std::string_view xmlNameSpace = "xmlns:";

		std::unordered_map<std::string_view, std::string_view> nameSpaces;
		std::stack<std::pair<size_t, std::pair<std::string_view, std::string_view>>> nameSpaceStack;

	public:
		static bool IsDeclaration(std::string_view const value)
		{
			return value.compare(0, xmlNameSpace.size(), xmlNameSpace) == 0;
		}

		static std::string_view CheckForDeclaration(std::string_view const value)
		{
			return IsDeclaration(value) ? value.substr(xmlNameSpace.size()) : std::string_view {};
		}

		[[nodiscard]] std::string_view Get(std::string_view const prefix) const
		{
			auto const nit = nameSpaces.find(prefix);
			if (nit == nameSpaces.end())
			{
				throw std::runtime_error("Namespace not found"); // +detail
			}
			return nit->second;
		}

		// rename?
		void Push(std::string_view const qualifiedName, std::string_view const value, const size_t depth)
		{
			std::string_view const prefix = CheckForDeclaration(qualifiedName);
			if (prefix.empty())
			{
				return;
			}

			auto const nit = nameSpaces.find(prefix);
			if (nit != nameSpaces.end())
			{
				nameSpaceStack.push({depth, {prefix, nit->second}});
				nit->second = value;
			}
			else
			{
				nameSpaces.emplace(prefix, value);
			}
		}

		void Pop(size_t const depth)
		{
			while (!nameSpaceStack.empty() && depth == nameSpaceStack.top().first)
			{
				auto nit = nameSpaces.find(nameSpaceStack.top().second.first);
				if (nit == nameSpaces.end())
				{
					throw std::logic_error("Inconsistent namespace stack");
				}
				nit->second = nameSpaceStack.top().second.second;
				nameSpaceStack.pop();
			}
		}

		static void ValidateName(size_t const colon, std::string_view const value)
		{
			if (colon == 0 || value.find(':', colon + 1) != std::string_view::npos)
			{
				throw std::runtime_error(std::string("Illegal name : '") + std::string(value) + '\'');
			}
		}

		[[nodiscard]] std::tuple<std::string_view, std::string_view> Normalise(std::string_view const name) const
		{
			size_t const colon = name.find(':');
			if (colon != std::string_view::npos)
			{
				ValidateName(colon, name);
				auto const prefix = name.substr(0, colon);
				auto const iter = nameSpaces.find(prefix);
				if (iter == nameSpaces.end())
				{
					throw std::runtime_error(std::string("NameSpace ") + std::string(prefix) + " not found");
				}
				return {name.substr(colon + 1), iter->second};
			}
			return {name, {}};
		}
	};
}