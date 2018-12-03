#include <boost/test/unit_test.hpp>

#include "TestInterfaces.h"

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
	GLib::Win::ComPtr<ITest2> p2(GLib::Win::Make<ImplementsITest2AndITest1>());
	GLib::Win::ComPtr<ITest1> p1;
	HRESULT hr = p2->QueryInterface(&p1);
	BOOST_TEST(S_OK == hr);
	BOOST_TEST(2U == p1.UseCount());
	BOOST_TEST(2U == p2.UseCount());

	GLib::Win::ComPtr<IUnknown> pu;
	hr = p2->QueryInterface(&pu);
	BOOST_TEST(S_OK == hr);
	BOOST_TEST(pu.Get() == p2.Get());
}

BOOST_AUTO_TEST_SUITE_END()