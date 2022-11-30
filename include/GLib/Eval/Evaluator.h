#pragma once

#include <GLib/Eval/Collection.h>
#include <GLib/Split.h>

#include <functional>
#include <stdexcept>

namespace GLib::Eval
{
	// move?
	template <typename T, std::enable_if_t<std::is_class<T>::value> * = nullptr>
	void VisitProperty(T const & value, std::string const & propertyName)
	{
		Visitor<T>::Visit(propertyName, value);
	}

	class Evaluator
	{
		std::unordered_map<std::string, ValuePtr> values;
		std::unordered_map<std::string, ValueBase const &> localValues;

	public:
		template <typename ValueType>
		void Set(std::string const & name, ValueType value)
		{
			ValuePtr v = MakeValue(value);
			auto const it = values.find(name);

			if (it == values.end())
			{
				values.emplace(name, move(v));
			}
			else
			{
				it->second = move(v);
			}
		}

		// specialise add with IsCollection? allow value types?
		template <typename Container>
		void SetCollection(std::string const & name, Container const & container)
		{
			auto v = std::make_unique<Collection<Container>>(container);
			auto const it = values.find(name);

			if (it == values.end())
			{
				values.emplace(name, move(v));
			}
			else
			{
				it->second = move(v);
			}
		}

		void Remove(std::string const & name)
		{
			auto const it = values.find(name);
			if (it == values.end())
			{
				throw std::runtime_error("Value not found : " + name);
			}
			values.erase(it);
		}

		void Push(std::string const & name, ValueBase const & value)
		{
			if (!localValues.emplace(name, value).second)
			{
				throw std::runtime_error(std::string("Local value already exists : ") + name);
			}
		}

		void Pop(std::string const & name)
		{
			if (localValues.erase(name) == 0)
			{
				throw std::runtime_error(std::string("Local value not found : ") + name);
			}
		}

		void ForEach(std::string const & name, ValueVisitor const & visitor) const
		{
			Evaluate(name, [&](ValueBase const & value) { value.ForEach(visitor); });
		}

		[[nodiscard]] std::string Evaluate(std::string const & name) const
		{
			std::string result;
			Evaluate(name, [&](ValueBase const & value) { result = value.ToString(); });
			return result;
		}

	private:
		void Evaluate(std::string const & value, ValueVisitor const & visitor) const
		{
			auto const & s = Util::Splitter {value, "."};
			auto it = s.begin();

			auto const lit = localValues.find(*it);
			if (lit != localValues.end())
			{
				++it;
				return SubEvaluate(lit->second, it, s.end(), visitor);
			}

			auto const vit = values.find(*it);
			if (vit == values.end())
			{
				throw std::runtime_error("Value not found : " + *it);
			}
			++it;
			SubEvaluate(*vit->second, it, s.end(), visitor);
		}

		static void SubEvaluate(ValueBase const & value, Util::Splitter::Iterator & it, Util::Splitter::Iterator const & end,
														ValueVisitor const & visitor)
		{
			if (it != end)
			{
				value.VisitProperty(*it,
														[&](ValueBase const & subValue)
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