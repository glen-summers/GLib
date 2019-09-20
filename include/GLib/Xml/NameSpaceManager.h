#pragma once

#include <stack>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace GLib::Xml
{
	class NameSpaceManager
	{
		static constexpr std::string_view Attribute = "xmlns:";

		std::unordered_map<std::string_view, std::string_view> nameSpaces;
		std::stack<std::pair<size_t, std::pair<std::string_view,std::string_view>>> nameSpaceStack;

	public:
		static bool IsDeclaration(const std::string_view & value)
		{
			return value.compare(0, Attribute.size(), Attribute) == 0;
		}

		static bool CheckForDeclaration(const std::string_view & value, std::string_view & prefix)
		{
			bool check = IsDeclaration(value);
			if (check)
			{
				prefix = value.substr(Attribute.size());
			}
			return check;
		}

		std::string_view Get(const std::string_view & prefix) const
		{
			auto nit = nameSpaces.find(prefix);
			if (nit==nameSpaces.end())
			{
				throw std::runtime_error("Namespace not found"); // +detail
			}
			return nit->second;
		}

		// rename?
		void Push(const std::string_view & qualifiedName, const std::string_view & value, size_t depth)
		{
			std::string_view prefix;
			if (!CheckForDeclaration(qualifiedName, prefix))
			{
				return;
			}

			auto nit = nameSpaces.find(prefix);
			if (nit!=nameSpaces.end())
			{
				nameSpaceStack.push({depth, {prefix, nit->second}});
				nit->second = value;
			}
			else
			{
				nameSpaces.emplace(prefix, value);
			}
		}

		void Pop(size_t depth)
		{
			while (!nameSpaceStack.empty() && depth == nameSpaceStack.top().first)
			{
				auto nit = nameSpaces.find(nameSpaceStack.top().second.first);
				if (nit==nameSpaces.end())
				{
					throw std::logic_error("Inconsistent namespace stack");
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
				auto prefix = name.substr(0, colon);
				auto it = nameSpaces.find(prefix);
				if (it == nameSpaces.end())
				{
					throw std::runtime_error(std::string("NameSpace ") + std::string(prefix) + " not found");
				}
				return { name.substr(colon+1), it->second };
			}
			return { name, {}};
		}
	};
}