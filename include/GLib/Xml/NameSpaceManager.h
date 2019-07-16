#pragma once

#include <string_view>
#include <unordered_map>
#include <stack>

namespace GLib::Xml
{
	class NameSpaceManager // name?
	{
		inline static constexpr std::string_view NameSpaceAttribute = "xmlns";

		std::unordered_map<std::string_view, std::string_view> nameSpaces;
		std::stack<std::pair<size_t, std::pair<std::string_view,std::string_view>>> nameSpaceStack;

	public:
		static bool IsNameSpace(const std::string_view & name)
		{
			return (name.compare(0, NameSpaceAttribute.size(), NameSpaceAttribute) == 0);
		}

		void Add(const std::string_view & name, const std::string_view & nameSpace) // hack, remove
		{
			auto nit = nameSpaces.find(name);
			if (nit!=nameSpaces.end())
			{
				if (nit->second != nameSpace)
				{
					throw std::runtime_error("namespace redefine");
				}
			}
			nameSpaces.emplace(name, nameSpace);
		}

		void Check(const std::string_view & qualifiedName, const std::string_view & value, size_t depth)
		{
			if (!IsNameSpace(qualifiedName))
			{
				return;
			}

			std::string_view nameSpace = value;
			std::string_view name = qualifiedName.substr(NameSpaceAttribute.size()+1);

			auto nit = nameSpaces.find(name);
			if (nit!=nameSpaces.end())
			{
				nameSpaceStack.push({depth, {name, nit->second}});
				nit->second = nameSpace;
			}
			else
			{
				nameSpaces.emplace(name, nameSpace);
			}
		}

		void Pop(size_t depth)
		{
			while (!nameSpaceStack.empty() && depth == nameSpaceStack.top().first)
			{
				auto nit = nameSpaces.find(nameSpaceStack.top().second.first);
				if (nit==nameSpaces.end())
				{
					throw std::logic_error("!");
				}
				nit->second = nameSpaceStack.top().second.second;
				nameSpaceStack.pop();
			}
		}

		static void ValidateName(size_t colon, const std::string_view & value)
		{
			if (colon == 0 || value.find(':', colon+1) != std::string_view::npos)
			{
				throw std::runtime_error(std::string("Illegal name : '") + std::string(value) + '\'');
			}
		}

		std::tuple<std::string_view, std::string_view> Normalise(const std::string_view & name) const
		{
			const size_t colon = name.find(':');
			if (colon != std::string_view::npos)
			{
				ValidateName(colon, name);
				auto nameSpacePrefix = name.substr(0, colon);
				auto it = nameSpaces.find(nameSpacePrefix);
				if (it == nameSpaces.end())
				{
					throw std::runtime_error(std::string("NameSpace ") + std::string(nameSpacePrefix) + " not found");
				}
				return { name.substr(colon+1), it->second };
			}
			return { name, {}};
		}
	};
}