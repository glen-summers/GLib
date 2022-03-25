#include <GLib/TypePredicates.h>

#include <boost/test/unit_test.hpp>

#include <typeindex>

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
	struct TypeIndex
	{
		friend bool operator==(const TypeIndex & lhs, const TypeIndex & rhs) = default;
		friend bool operator!=(const TypeIndex & lhs, const TypeIndex & rhs) = default;
		std::type_index value;
	};

	std::ostream & operator<<(std::ostream & s, const TypeIndex & ti)
	{
		return s << ti.value.name();
	}

	template <typename First, typename... Rest>
	struct ToTypeId;

	template <typename First>
	struct ToTypeId<GLib::Util::TypeList<First>>
	{
		static void Append(std::list<TypeIndex> & l)
		{
			l.emplace_back(typeid(First));
		}
	};

	template <typename First, typename Second>
	struct ToTypeId<GLib::Util::TypeList<First, Second>>
	{
		static void Append(std::list<TypeIndex> & l)
		{
			l.emplace_back(typeid(First));
			l.emplace_back(typeid(Second));
		}
	};

	template <typename First, typename Second, typename... Rest>
	struct ToTypeId<GLib::Util::TypeList<First, Second, Rest...>>
	{
		static void Append(std::list<TypeIndex> & l)
		{
			l.emplace_back(typeid(First));
			ToTypeId<GLib::Util::TypeList<Second, Rest...>>::Append(l);
		}
	};
}

BOOST_AUTO_TEST_SUITE(TypeFilterTests)

BOOST_AUTO_TEST_CASE(TestTypeListCompare)
{
	std::list<Util::TypeIndex> expected {{typeid(int)}, {typeid(long)}};

	std::list<Util::TypeIndex> actual;
	Util::ToTypeId<GLib::Util::Tuple<int, long>::Type>::Append(actual);

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestIntegralFilter)
{
	using Result = GLib::Util::TypeFilter<std::is_integral, int, float, long>::TupleType::Type;
	std::list<Util::TypeIndex> expected {{typeid(int)}, {typeid(long)}};

	std::list<Util::TypeIndex> actual;
	Util::ToTypeId<Result>::Append(actual);
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestInterfaceFilter)
{
	using Result = GLib::Util::TypeFilter<Predicates::IsDerivedFromFoo, Interfaces::IFoo, Interfaces::IBar, Interfaces::IFooDerived>::TupleType::Type;
	std::list<Util::TypeIndex> expected {{typeid(Interfaces::IFooDerived)}};

	std::list<Util::TypeIndex> actual;
	Util::ToTypeId<Result>::Append(actual);
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestSelfFilter)
{
	using Result =
		GLib::Util::SelfTypeFilter<GLib::TypePredicates::HasNoInheritor, Interfaces::IFoo, Interfaces::IFooDerived, Interfaces::IBar>::TupleType::Type;
	std::list<Util::TypeIndex> expected {{typeid(Interfaces::IFooDerived)}, {typeid(Interfaces::IBar)}};

	std::list<Util::TypeIndex> actual;
	Util::ToTypeId<Result>::Append(actual);
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_SUITE_END()
