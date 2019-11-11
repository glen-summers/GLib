#pragma once

#include "GLib/Eval/Value.h"

namespace GLib::Eval
{
	// try with just value no collection, just use ForEach specialisation
	struct CollectionBase : ValueBase
	{
		CollectionBase() = default;
		CollectionBase(const CollectionBase &) = delete;
		CollectionBase(CollectionBase &&) = delete;
		CollectionBase & operator=(const CollectionBase &) = delete;
		CollectionBase & operator=(CollectionBase &&) = delete;
		~CollectionBase() override = default;
	};

	template<typename Container> class Collection : public CollectionBase
	{
		const Container & container;

	public:
		Collection(const Container & container)
			: container(container)
		{
		}

	protected:
		std::string ToString() const override
		{
			std::ostringstream stm;
			auto it = container.begin(), end = container.end();
			if (it != end)
			{
				stm << Utils::ToString(*it++);
			}
			while (it != end)
			{
				stm << "," << Utils::ToString(*it++);
			}
			return stm.str();
		}

		void VisitProperty(const std::string & propertyName, const ValueVisitor & visitor) const override
		{
			(void)propertyName;
			(void)visitor;
			throw std::runtime_error("Not implemented");
			// size?
		}

		void ForEach(const ValueVisitor & f) const override
		{
			for (const auto & value : container)
			{
				f(Value<typename Container::value_type> { value });
			}
		}
	};

	template <typename T> Collection<T> MakeCollection(const T & container)
	{
		return Collection<T>(container);
	}
}