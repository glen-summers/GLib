#pragma once

#include <GLib/Compat.h>
#include <GLib/Eval/Utils.h>

#include <functional>
#include <memory>
#include <string>

namespace GLib::Eval
{
	struct ValueBase;
	using ValuePtr = std::unique_ptr<ValueBase>;
	using ValueVisitor = std::function<void(ValueBase const &)>;

	template <typename ValueType>
	class Value;

	template <typename Value>
	struct Visitor;

	template <typename ValueType>
	ValuePtr MakeValue(ValueType value)
	{
		return std::make_unique<Value<ValueType>>(value);
	}

	template <typename T, std::enable_if_t<!Utils::Detail::IsContainer<T>::value> * = nullptr>
	void ForEach(T const value, ValueVisitor const & visitor)
	{
		static_cast<void>(value);
		static_cast<void>(visitor);
		throw std::runtime_error(std::string("ForEach not defined for : ") + Compat::Unmangle(typeid(T).name()));
	}

	template <typename T, std::enable_if_t<Utils::Detail::IsContainer<T>::value> * = nullptr>
	void ForEach(T const collection, ValueVisitor const & visitor)
	{
		for (auto const & value : collection)
		{
			visitor(Value(value));
		}
	}

	struct ValueBase
	{
		ValueBase() = default;
		ValueBase(ValueBase const &) = delete;
		ValueBase(ValueBase &&) = delete;
		ValueBase & operator=(ValueBase const &) = delete;
		ValueBase & operator=(ValueBase &&) = delete;
		virtual ~ValueBase() = default;

		[[nodiscard]] virtual std::string ToString() const = 0; // +format/stream?

		virtual void VisitProperty(std::string const & propertyName, ValueVisitor const & visitor) const = 0;
		virtual void ForEach(ValueVisitor const & visitor) const = 0;
	};

	template <typename ValueType>
	class Value : public ValueBase
	{
		ValueType const value;

	public:
		explicit Value(ValueType value)
			: value(std::move(value))
		{}

		[[nodiscard]] std::string ToString() const override
		{
			return Utils::ToString(value);
		}

		void VisitProperty(std::string const & propertyName, ValueVisitor const & visitor) const override
		{
			Visitor<ValueType>::Visit(value, propertyName, visitor);
		}

		void ForEach(ValueVisitor const & visitor) const override
		{
			return Eval::ForEach(value, visitor);
		}
	};

	template <typename Value>
	struct Visitor
	{
		static void Visit(Value const & value, std::string const & propertyName, ValueVisitor const & visitor)
		{
			static_cast<void>(value);
			static_cast<void>(visitor);

			throw std::runtime_error(std::string("No accessor defined for property: '") + propertyName + "', type:'" +
															 Compat::Unmangle(typeid(Value).name()) + '\'');
		}
	};
}