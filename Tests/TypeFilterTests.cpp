#include <GLib/TypePredicates.h>

#include <boost/test/unit_test.hpp>

#include <typeindex>

#include "TestUtils.h"

using GLib::Util::Tuple;
using GLib::Util::TypeFilter;

namespace Interfaces
{
	struct IFoo
	{
		IFoo() = default;
		IFoo(const IFoo &) = delete;
		IFoo(IFoo &&) = delete;
		IFoo & operator=(const IFoo &) = delete;
		IFoo & operator=(IFoo &&) = delete;
		virtual ~IFoo() = default;

		virtual void MethodFoo() = 0;
	};

	struct IFooDerived : IFoo
	{
		IFooDerived() = default;
		IFooDerived(const IFooDerived &) = delete;
		IFooDerived(IFooDerived &&) = delete;
		IFooDerived & operator=(const IFooDerived &) = delete;
		IFooDerived & operator=(IFooDerived &&) = delete;
		~IFooDerived() override = default;

		virtual void MethodFooDerived() = 0;
	};

	struct IBar
	{
		IBar() = default;
		IBar(const IBar &) = delete;
		IBar(IBar &&) = delete;
		IBar & operator=(const IBar &) = delete;
		IBar & operator=(IBar &&) = delete;
		virtual ~IBar() = default;

		virtual void MethodBar() = 0;
	};

	struct IBaz
	{
		IBaz() = default;
		IBaz(const IBaz &) = delete;
		IBaz(IBaz &&) = delete;
		IBaz & operator=(const IBaz &) = delete;
		IBaz & operator=(IBaz &&) = delete;
		virtual ~IBaz() = default;

		virtual void MethodBaz() = 0;
	};
}

namespace Predicates
{
	template <typename T>
	using IsDerivedFromFoo = GLib::TypePredicates::IsDerivedFrom<T, Interfaces::IFoo>;
}

namespace Util
{
	class TypeIndex
	{
		std::type_index value;

	public:
		explicit TypeIndex(std::type_index value)
			: value {value}
		{}

		friend bool operator==(const TypeIndex & lhs, const TypeIndex & rhs) = default;
		friend bool operator!=(const TypeIndex & lhs, const TypeIndex & rhs) = default;

		[[nodiscard]] std::type_index Value() const
		{
			return value;
		}
	};

	std::ostream & operator<<(std::ostream & s, const TypeIndex & ti)
	{
		return s << ti.Value().name();
	}

	template <typename First, typename... Rest>
	struct ToTypeId;

	template <typename First>
	struct ToTypeId<Tuple<First>>
	{
		static void Append(std::list<TypeIndex> & l)
		{
			l.emplace_back(typeid(First));
		}
	};

	template <typename First, typename Second>
	struct ToTypeId<Tuple<First, Second>>
	{
		static void Append(std::list<TypeIndex> & l)
		{
			l.emplace_back(typeid(First));
			l.emplace_back(typeid(Second));
		}
	};

	template <typename First, typename Second, typename... Rest>
	struct ToTypeId<Tuple<First, Second, Rest...>>
	{
		static void Append(std::list<TypeIndex> & l)
		{
			l.emplace_back(typeid(First));
			ToTypeId<Tuple<Second, Rest...>>::Append(l);
		}
	};

	template <typename T>
	TypeIndex Index()
	{
		return TypeIndex {typeid(T)};
	}
}

using Util::Index;
using Util::ToTypeId;

AUTO_TEST_SUITE(TypeFilterTests)

AUTO_TEST_CASE(TestTypeListCompare)
{
	std::list expected {Index<int>(), Index<long>()};

	std::list<Util::TypeIndex> actual;
	ToTypeId<Tuple<int, long>::Type>::Append(actual);

	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

AUTO_TEST_CASE(TestIntegralFilter)
{
	using Result = TypeFilter<std::is_integral, int, float, long>::TupleType::Type;
	std::list expected {Index<int>(), Index<long>()};

	std::list<Util::TypeIndex> actual;
	ToTypeId<Result>::Append(actual);
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

AUTO_TEST_CASE(TestInterfaceFilter)
{
	using Result = TypeFilter<Predicates::IsDerivedFromFoo, Interfaces::IFoo, Interfaces::IBar, Interfaces::IFooDerived>::TupleType::Type;
	std::list expected {Index<Interfaces::IFooDerived>()};

	std::list<Util::TypeIndex> actual;
	ToTypeId<Result>::Append(actual);
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

AUTO_TEST_CASE(TestSelfFilter)
{
	using Result =
		GLib::Util::SelfTypeFilter<GLib::TypePredicates::HasNoInheritor, Interfaces::IFoo, Interfaces::IFooDerived, Interfaces::IBar>::TupleType::Type;
	std::list expected {Index<Interfaces::IFooDerived>(), Index<Interfaces::IBar>()};

	std::list<Util::TypeIndex> actual;
	ToTypeId<Result>::Append(actual);
	CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

AUTO_TEST_SUITE_END()
