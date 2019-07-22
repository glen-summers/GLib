#pragma once

#include "GLib/Eval/Utils.h"
#include "GLib/compat.h"

#include <memory>
#include <string>
#include <functional>

namespace GLib::Eval
{
	struct ValueBase;
	typedef std::unique_ptr<ValueBase> ValuePtr;
	typedef std::function<void(const ValueBase &)> ValueVisitor;
	template<typename ValueType> struct Value;
	template<typename Value> struct Visitor;

	template <typename ValueType> ValuePtr MakeValue(ValueType value)
	{
		return std::make_unique<Value<ValueType>>(value);
	}

	template <typename T, std::enable_if_t<!Utils::Detail::IsContainer<T>::value>* = nullptr>
	void ForEach(T value, const ValueVisitor & f)
	{
		(void)value;
		(void)f;
		throw std::runtime_error(std::string("ForEach not defined for : ") + Compat::Unmangle(typeid(T).name()));
	}

	template <typename T, std::enable_if_t<Utils::Detail::IsContainer<T>::value>* = nullptr>
	void ForEach(T collection, const ValueVisitor & f)
	{
		for (const auto & value : collection)
		{
			f(Value(value));
		}
	}

	struct ValueBase
	{
		ValueBase() = default;
		ValueBase(const ValueBase &) = delete;
		ValueBase(ValueBase &&) = delete;
		ValueBase & operator=(const ValueBase &) = delete;
		ValueBase & operator=(ValueBase &&) = delete;
		virtual ~ValueBase() = default;

		virtual std::string ToString() const = 0; // +format, or ostream& ?

		virtual void VisitProperty(const std::string & propertyName, const ValueVisitor & f) const = 0;
		virtual void ForEach(const ValueVisitor & f) const = 0;
	};

	template<typename ValueType> struct Value : ValueBase
	{
		ValueType value;

		Value(ValueType value)
			: value(std::move(value))
		{
		}

		std::string ToString() const override
		{
			return Utils::ToString(value);
		}

		void VisitProperty(const std::string & propertyName, const ValueVisitor & visitor) const override
		{
			Visitor<ValueType>::Visit(value, propertyName, visitor);
		}

		void ForEach(const ValueVisitor & f) const override
		{
			return GLib::Eval::ForEach(value, f);
		}
	};

	template<typename Value> struct Visitor
	{
		static std::string Visit(const Value & value, const std::string & propertyName, const ValueVisitor & visitor)
		{
			(void)value;
			(void)visitor;

			throw std::runtime_error(std::string("No accessor defined for property: '")
				+ propertyName + "', type:'" + Compat::Unmangle(typeid(Value).name()) + '\'');
		}
	};
}