#include <boost/test/unit_test.hpp>

#include <GLib/TypeFilter.h>
#include <GLib/compat.h>

#include <type_traits>

namespace Interfaces
{
	struct IFoo
	{
		virtual ~IFoo() = default;
		virtual void MethodFoo() = 0;
	};

	struct IFoo2 : IFoo
	{
		virtual ~IFoo2() = default;
		virtual void MethodFoo2() = 0;
	};

	struct IBar
	{
		virtual ~IBar() = default;
		virtual void MethodBar() = 0;
	};

	struct IBaz
	{
		virtual ~IBaz() = default;
		virtual void MethodBaz() = 0;
	};
}

namespace Predicates
{
	template<class T1, class T2>
	struct IsBaseOf : std::bool_constant<std::is_base_of<T1, T2>::value
			&& !std::is_same<T1, T2>::value>
	{};
	
	template<class T1, class T2> using IsDerivedFrom = IsBaseOf<T2, T1>;

	template <typename T> using IsDerivedFromFoo = IsDerivedFrom<T, Interfaces::IFoo>;

	template<class T, class... Types> struct IsBaseOfAny;

	template<class T> struct IsBaseOfAny<T> : std::bool_constant<false>
	{};

	template<class T, class First, class... Rest>
	struct IsBaseOfAny<T, First, Rest...>
		: std::bool_constant<IsBaseOf<T, First>::value || IsBaseOfAny<T, Rest...>::value>
	{};

	template<class T, class... Types> struct HasNoInheritor;

	template<class T> struct HasNoInheritor<T> : std::bool_constant<false>
	{};

	template<class T, class First, class... Rest>
	struct HasNoInheritor<T, First, Rest...>
		: std::bool_constant<!IsBaseOf<T, First>::value && !IsBaseOfAny<T, Rest...>::value>
	{};
}

namespace Util
{
	template<typename First, typename... Rest> struct ToTypeId;

	template<typename First> struct ToTypeId<GLib::Util::TypeList<First>>
	{
		static void Append(std::list<std::type_index> & l)
		{
			 l.push_back(typeid(First));
		}
	};

	template<typename First, typename Second>
	struct ToTypeId<GLib::Util::TypeList<First, Second>>
	{
		static void Append(std::list<std::type_index> & l)
		{
			l.emplace_back(typeid(First));
			l.emplace_back(typeid(Second));
		}
	};

	template<typename First, typename Second, typename... Rest>
	struct ToTypeId<GLib::Util::TypeList<First, Second, Rest...>>
	{
		static void Append(std::list<std::type_index> & l)
		{
			l.emplace_back(typeid(First));
			ToTypeId<GLib::Util::TypeList<Second, Rest...>>::Append(l);
		}
	};
}

namespace std
{
	std::ostream & operator<<(std::ostream & s, const std::type_index & ti)
	{
		return s << ti.name();
	}
}

BOOST_AUTO_TEST_SUITE(TypeFilterTests)

BOOST_AUTO_TEST_CASE(TestTypeListCompare)
{
	std::list<std::type_index> expected { typeid(int), typeid(long) };

	std::list<std::type_index> actual;
	Util::ToTypeId<GLib::Util::Tuple<int, long>::Type>::Append(actual);

	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestIntegralFilter)
{
	typedef GLib::Util::TypeFilter<std::is_integral, int, float, long>::TupleType::Type Result;
	std::list<std::type_index> expected { typeid(int), typeid(long) };

	std::list<std::type_index> actual;
	Util::ToTypeId<Result>::Append(actual);
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestInterfaceFilter)
{
	typedef GLib::Util::TypeFilter<Predicates::IsDerivedFromFoo,
			Interfaces::IFoo, Interfaces::IBar, Interfaces::IFoo2>::TupleType::Type Result;
	std::list<std::type_index> expected { typeid(Interfaces::IFoo2) };

	std::list<std::type_index> actual;
	Util::ToTypeId<Result>::Append(actual);
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_CASE(TestSelfFilter)
{
	typedef GLib::Util::SelfTypeFilter<Predicates::HasNoInheritor,
		Interfaces::IFoo, Interfaces::IFoo2, Interfaces::IBar>::TupleType::Type Result;
	std::list<std::type_index> expected { typeid(Interfaces::IFoo2),typeid(Interfaces::IBar) };

	std::list<std::type_index> actual;
	Util::ToTypeId<Result>::Append(actual);
	BOOST_CHECK_EQUAL_COLLECTIONS(expected.begin(), expected.end(), actual.begin(), actual.end());
}

BOOST_AUTO_TEST_SUITE_END()
