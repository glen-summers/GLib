#pragma once

#include "GLib/Eval/Collection.h"
#include "GLib/split.h"

#include <functional>
#include <stdexcept>

namespace GLib::Eval
{
	// move?
	template <typename T, std::enable_if_t<std::is_class<T>::value>* = nullptr>
	void VisitProperty(const T & value, const std::string & propertyName)
	{
		return Visitor<T>::Visit(propertyName, value);
	}

	class Evaluator
	{
		std::unordered_map<std::string, ValuePtr> values;
		std::unordered_map<std::string, const ValueBase &> localValues;

	public:
		template<typename ValueType> void Add(const std::string & name, ValueType value)
		{
			if (!values.emplace(name, MakeValue(value)).second)
			{
				throw std::runtime_error(std::string("Value already exists : ") + name);
			}
		}

		// specialise add with IsCollection?
		template<typename Container> void AddCollection(const std::string & name, const Container & container)
		{
			if (!values.emplace(name, std::make_unique<Collection<Container>>(container)).second)
			{
				throw std::runtime_error(std::string("Value already exists : ") + name);
			}
		}

		void Remove(const std::string & name)
		{
			const auto it = values.find(name);
			if (it == values.end())
			{
				throw std::runtime_error("Value not found : " + name);
			}
			values.erase(it);
		}

		void Push(const std::string & name, const ValueBase & value)
		{
			if (!localValues.emplace(name, value).second)
			{
				throw std::runtime_error(std::string("Local value already exists : ") + name);
			}
		}

		void Pop(const std::string & name)
		{
			if (localValues.erase(name) == 0)
			{
				throw std::runtime_error(std::string("Local value not found : ") + name);
			}
		}

		void ForEach(const std::string & name, const ValueVisitor & visitor) const
		{
			Evaluate(name, [&](const ValueBase & value)
			{
				value.ForEach(visitor);
			});
		}

		std::string Evaluate(const std::string & name) const
		{
			std::string result;
			Evaluate(name, [&](const ValueBase & value)
			{
				result = value.ToString();
			});
			return result;
		}

	private:
		void Evaluate(const std::string & value, const ValueVisitor & visitor) const
		{
			const auto & s = GLib::Util::Splitter{value, "."};
			auto it = s.begin();

			const auto lit = localValues.find(*it);
			if (lit != localValues.end())
			{
				++it;
				return SubEvaluate(lit->second, it, s.end(), visitor);
			}

			const auto vit = values.find(*it);
			if (vit == values.end())
			{
				throw std::runtime_error("Value not found : " + *it);
			}
			++it;
			SubEvaluate(*vit->second, it, s.end(), visitor);
		}

		static void SubEvaluate(const ValueBase & value, GLib::Util::Splitter::iterator & it,
			const GLib::Util::Splitter::iterator & end, const ValueVisitor & visitor)
		{
			if (it!=end)
			{
				value.VisitProperty(*it, [&](const ValueBase & subValue)
				{
					++it;
					SubEvaluate(subValue, it, end, visitor);
				});
			}
			else
			{
				visitor(value);
			}
		}
	};
}