#pragma once

#include <GLib/Eval/Collection.h>
#include <GLib/Split.h>

#include <functional>
#include <stdexcept>

namespace GLib::Eval
{
	// move?
	template <typename T, std::enable_if_t<std::is_class_v<T>> * = nullptr>
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
			ValuePtr valuePtr = MakeValue(value);
			auto const iter = values.find(name);

			if (iter == values.end())
			{
				values.emplace(name, std::move(valuePtr));
			}
			else
			{
				iter->second = std::move(valuePtr);
			}
		}

		// specialise add with IsCollection? allow value types?
		template <typename Container>
		void SetCollection(std::string const & name, Container const & container)
		{
			auto value = std::make_unique<Collection<Container>>(container);
			auto const iter = values.find(name);

			if (iter == values.end())
			{
				values.emplace(name, std::move(value));
			}
			else
			{
				iter->second = std::move(value);
			}
		}

		void Remove(std::string const & name)
		{
			auto const iter = values.find(name);
			if (iter == values.end())
			{
				throw std::runtime_error("Value not found : " + name);
			}
			values.erase(iter);
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
			auto const & splitValue = Util::Splitter {value, "."};
			auto iter = splitValue.begin();

			auto const localIt = localValues.find(*iter);
			if (localIt != localValues.end())
			{
				++iter;
				return SubEvaluate(localIt->second, iter, splitValue.end(), visitor);
			}

			auto const valueIt = values.find(*iter);
			if (valueIt == values.end())
			{
				throw std::runtime_error("Value not found : " + *iter);
			}
			++iter;
			SubEvaluate(*valueIt->second, iter, splitValue.end(), visitor);
		}

		static void SubEvaluate(ValueBase const & value, Util::Splitter::Iterator & iter, Util::Splitter::Iterator const & end,
														ValueVisitor const & visitor)
		{
			if (iter != end)
			{
				value.VisitProperty(*iter,
														[&](ValueBase const & subValue)
														{
															++iter;
															SubEvaluate(subValue, iter, end, visitor);
														});
			}
			else
			{
				visitor(value);
			}
		}
	};
}