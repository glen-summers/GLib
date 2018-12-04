#include <boost/test/unit_test.hpp>

#include "TestInterfaces.h"

#include "GLib/Win/ComUtil.h"

BOOST_AUTO_TEST_SUITE(ComPtrTests)

	BOOST_AUTO_TEST_CASE(UninitialisedComPtrHasZeroUseCount)
	{
		GLib::Win::ComPtr<ITest1> p;
		BOOST_TEST(0U == p.UseCount());
		BOOST_TEST(false == static_cast<bool>(p));
		BOOST_TEST(nullptr == p.Get());
	}

	BOOST_AUTO_TEST_CASE(NullInitialisedComPtrHasZeroUseCount)
	{
		GLib::Win::ComPtr<ITest1> p1(nullptr);
		BOOST_TEST(0U == p1.UseCount());
		BOOST_TEST(false == static_cast<bool>(p1));
		BOOST_TEST(nullptr == p1.Get());

		GLib::Win::ComPtr<ITest1> p2 = nullptr;
		BOOST_TEST(0U == p2.UseCount());
		BOOST_TEST(false == static_cast<bool>(p2));
		BOOST_TEST(nullptr == p2.Get());
	}

	BOOST_AUTO_TEST_CASE(TestInitialisedComPtrHasOneUseCount)
	{
		GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1>());

		BOOST_TEST(1U == p1.UseCount());
		BOOST_TEST(nullptr != p1.Get());
	}

	BOOST_AUTO_TEST_CASE(TestCtorFromSameType)
	{
		GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1>());
		GLib::Win::ComPtr<ITest1> p2(p1);

		BOOST_TEST(2U == p1.UseCount());
		BOOST_TEST(2U, p2.UseCount());

		GLib::Win::ComPtr<ITest1> p3 = p1;
		BOOST_TEST(3U == p1.UseCount());
		BOOST_TEST(3U == p2.UseCount());
		BOOST_TEST(3U == p3.UseCount());
	}

	BOOST_AUTO_TEST_CASE(CallMethod)
	{
		GLib::Win::ComPtr<ITest1> test(GLib::Win::Make<ImplementsITest1>());
		BOOST_TEST(S_OK == test->TestMethod1());
	}

	BOOST_AUTO_TEST_CASE(TestReset)
	{
		ImplementsITest1::DeleteCount();

		GLib::Win::ComPtr<ITest1> p(GLib::Win::Make<ImplementsITest1>());
		BOOST_TEST(1U, p.UseCount());
		p.Reset();

		BOOST_TEST(0U == p.UseCount());
		BOOST_TEST(false == static_cast<bool>(p));
		BOOST_TEST(nullptr == p.Get());
		BOOST_TEST(1 == ImplementsITest1::DeleteCount());
	}

	BOOST_AUTO_TEST_CASE(TestQINoInterface)
	{
		GLib::Win::ComPtr<ITest1> p(GLib::Win::Make<ImplementsITest1>());
		GLib::Win::ComPtr<ITest2> p2;
		HRESULT hr = p->QueryInterface(&p2);
		BOOST_TEST(E_NOINTERFACE == hr);
	}

	BOOST_AUTO_TEST_CASE(TestQIOk)
	{
		GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1AndITest2>());
		GLib::Win::ComPtr<ITest1> p2;
		HRESULT hr = p1->QueryInterface(&p2);
		BOOST_TEST(S_OK == hr);
		BOOST_TEST(2U == p1.UseCount());
		BOOST_TEST(2U == p2.UseCount());

		GLib::Win::ComPtr<IUnknown> pu;
		hr = p2->QueryInterface(&pu);
		BOOST_TEST(S_OK == hr);
		BOOST_TEST(pu.Get() == p2.Get());
	}

	BOOST_AUTO_TEST_CASE(TestcanHoldConcreteClass)
	{
		const GLib::Win::ComPtr<ImplementsITest1> p = GLib::Win::MakeConcrete<ImplementsITest1>();
		p->ConcreteMethod();
	}

	BOOST_AUTO_TEST_CASE(TestMultipleInheritance)
	{
		GLib::Win::ComPtr<ITest12> p12 = GLib::Win::Make<ImplementsITest12AndITest13>();
		p12->TestMethod1();
		p12->TestMethod12();

		GLib::Win::ComPtr<ITest13> p13;
		HRESULT hr = p12->QueryInterface(&p13);
		BOOST_TEST(S_OK == hr);
		p13->TestMethod1();
		p13->TestMethod13();
	}

	BOOST_AUTO_TEST_CASE(TestComCast)
	{
		GLib::Win::ComPtr<ITest12> p12 = GLib::Win::Make<ImplementsITest12AndITest13>();
		const auto p13 = GLib::Win::ComCast<ITest13>(p12);
		//BOOST_TEST(static_cast<bool>(GLib::Win::ComCast<ITest1>(p12)));
		//BOOST_TEST(static_cast<bool>(GLib::Win::ComCast<ITest2>(p12)));
		//BOOST_TEST(static_cast<bool>(GLib::Win::ComCast<ITest3>(p12)));
		BOOST_TEST(static_cast<bool>(GLib::Win::ComCast<ITest12>(p12)));
		BOOST_TEST(static_cast<bool>(GLib::Win::ComCast<ITest13>(p12)));
	}

	BOOST_AUTO_TEST_CASE(TestComFail)
	{
		GLib::Win::ComPtr<ITest12> p12 = GLib::Win::Make<ImplementsITest12AndITest13>();
		BOOST_TEST(static_cast<bool>(GLib::Win::ComCast<ITest13>(p12)));
	}

BOOST_AUTO_TEST_SUITE_END()