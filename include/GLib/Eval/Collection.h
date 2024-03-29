#pragma once

#include <GLib/Eval/Value.h>

namespace GLib::Eval
{
	// try with just value no collection, just use ForEach specialisation
	struct CollectionBase : ValueBase
	{
		CollectionBase() = default;
		CollectionBase(CollectionBase const &) = delete;
		CollectionBase(CollectionBase &&) = delete;
		CollectionBase & operator=(CollectionBase const &) = delete;
		CollectionBase & operator=(CollectionBase &&) = delete;

		~CollectionBase() override = default;
	};

	template <typename Container>
	class Collection : public CollectionBase
	{
		Container const & container;

	public:
		explicit Collection(Container const & container)
			: container(container)
		{}

	protected:
		[[nodiscard]] std::string ToString() const override
		{
			std::ostringstream stm;
			auto iter = container.begin();
			auto const end = container.end();
			if (iter != end)
			{
				stm << Utils::ToString(*iter++);
			}
			while (iter != end)
			{
				stm << "," << Utils::ToString(*iter++);
			}
			return stm.str();
		}

		void VisitProperty(std::string const & propertyName, ValueVisitor const & visitor) const override
		{
			static_cast<void>(propertyName);
			static_cast<void>(visitor);
			throw std::runtime_error("Not implemented");
			// size?
		}

		void ForEach(ValueVisitor const & visitor) const override
		{
			for (auto const & value : container)
			{
				visitor(Value<typename Container::value_type> {value});
			}
		}
	};

	template <typename T>
	Collection<T> MakeCollection(T const & container)
	{
		return Collection<T>(container);
	}
}
