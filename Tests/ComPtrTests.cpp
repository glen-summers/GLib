#include <boost/test/unit_test.hpp>

#ifdef _DEBUG
#define COM_PTR_DEBUG
#define SIMPLECOM_LOG_QI_MISS
#endif

#include "TestInterfaces.h"
#include "TestUtils.h"

namespace
{
	template <typename T> unsigned int UseCount(const GLib::Win::ComPtr<T> & p)
	{
		auto & ptr = const_cast<GLib::Win::ComPtr<T> &>(p);
		return !ptr ? 0 : (Get(ptr)->AddRef(), Get(ptr)->Release());
	}
}

namespace boost::test_tools::tt_detail
{
	template <typename T>
	struct print_log_value<GLib::Win::ComPtr<T>>
	{
		inline void operator()(std::ostream & str, const GLib::Win::ComPtr<T> & item)
		{
			str << "ptr: " << Get(item) << ", Ref: " << UseCount(item);
		}
	};
}

BOOST_AUTO_TEST_SUITE(ComPtrTests)

BOOST_AUTO_TEST_CASE(ComErrorCheckWithErrorInfo)
{
	GLib::Win::ComPtr<ICreateErrorInfo> p;
	GLib::Win::CheckHr(::CreateErrorInfo(GetAddress(p)), "CreateErrorInfo");
	auto ei = GLib::Win::ComCast<IErrorInfo>(p);
	p->SetDescription(const_cast<LPOLESTR>(L"hello"));
	GLib::Win::CheckHr(::SetErrorInfo(0, Get(ei)), "SetErrorInfo");

	GLIB_CHECK_EXCEPTION(GLib::Win::CheckHr(E_OUTOFMEMORY, "fail"),
		GLib::Win::ComException, "fail : hello (8007000E)");
}

BOOST_AUTO_TEST_CASE(ComErrorCheck)
{
	::SetErrorInfo(0, nullptr);
	GLIB_CHECK_EXCEPTION(GLib::Win::CheckHr(E_FAIL, "test E_FAIL"),
		GLib::Win::ComException, "test E_FAIL : Unspecified error (80004005)");
}

BOOST_AUTO_TEST_CASE(ComErrorCheck2)
{
	::SetErrorInfo(0, nullptr);
	GLIB_CHECK_EXCEPTION(GLib::Win::CheckHr(E_UNEXPECTED, "test E_UNEXPECTED"),
		GLib::Win::ComException, "test E_UNEXPECTED : Catastrophic failure (8000FFFF)");
}

BOOST_AUTO_TEST_CASE(UninitialisedComPtrHasZeroUseCount)
{
	GLib::Win::ComPtr<ITest1> p;
	BOOST_TEST(0U == UseCount(p));
	BOOST_TEST(false == static_cast<bool>(p));
	BOOST_TEST(nullptr == p);
	BOOST_TEST(p == nullptr);
	BOOST_TEST(nullptr == Get(p));
}

BOOST_AUTO_TEST_CASE(NullInitialisedComPtrHasZeroUseCount)
{
	GLib::Win::ComPtr<ITest1> p1(nullptr);
	BOOST_TEST(0U == UseCount(p1));
	BOOST_TEST(false == static_cast<bool>(p1));
	BOOST_TEST(nullptr == Get(p1));

	GLib::Win::ComPtr<ITest1> p2 = {};
	BOOST_TEST(0U == UseCount(p2));
	BOOST_TEST(false == static_cast<bool>(p2));
	BOOST_TEST(nullptr == Get(p2));
}

BOOST_AUTO_TEST_CASE(InitialisedComPtrHasOneUseCount)
{
	GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1>());

	BOOST_TEST(1U == UseCount(p1));
	BOOST_TEST(nullptr != p1);
	BOOST_TEST(p1 != nullptr);
	BOOST_TEST(nullptr != Get(p1));
}

BOOST_AUTO_TEST_CASE(CtorFromSameType)
{
	GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1>());
	BOOST_TEST(1U == UseCount(p1));

	GLib::Win::ComPtr<ITest1> p2(p1);
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));

	GLib::Win::ComPtr<ITest1> p2m(std::move(p2));
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(0U == UseCount(p2));
	BOOST_TEST(2U == UseCount(p2m));
}

BOOST_AUTO_TEST_CASE(CtorFromRawValue)
{
	GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1>());
	GLib::Win::ComPtr<ITest1> p2(Get(p1));
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));
}

BOOST_AUTO_TEST_CASE(CtorFromOtherType)
{
	GLib::Win::ComPtr<ITest1Extended> p1(GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>());
	BOOST_TEST(1U == UseCount(p1));

	GLib::Win::ComPtr<ITest1> p2(p1);
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));

	GLib::Win::ComPtr<ITest1> p2m(std::move(p2));
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(0U == UseCount(p2));
	BOOST_TEST(2U == UseCount(p2m));
}

BOOST_AUTO_TEST_CASE(AssignmentSameType)
{
	GLib::Win::ComPtr<ITest1> p1, p2, p3;
	p1 = GLib::Win::Make<ImplementsITest1>();
	BOOST_TEST(1U == UseCount(p1));
	BOOST_TEST(0U == UseCount(p2));
	BOOST_TEST(0U == UseCount(p3));

	p2 = p1;
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));
	BOOST_TEST(0U == UseCount(p3));

	p3 = std::move(p1);
	BOOST_TEST(0U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));
	BOOST_TEST(2U == UseCount(p3));
}

BOOST_AUTO_TEST_CASE(AssignmentOtherType)
{
	GLib::Win::ComPtr<ITest1Extended> p1(GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>());
	GLib::Win::ComPtr<ITest1> p2, p3;
	BOOST_TEST(1U == UseCount(p1));
	BOOST_TEST(0U == UseCount(p2));
	BOOST_TEST(0U == UseCount(p3));

	p2 = p1;
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));
	BOOST_TEST(0U == UseCount(p3));

	p3 = std::move(p1);
	BOOST_TEST(0U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));
	BOOST_TEST(2U == UseCount(p3));
}

BOOST_AUTO_TEST_CASE(SelfAssign)
{
	GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1>());
	GLib::Win::ComPtr<ITest1> p2(GLib::Win::Make<ImplementsITest1>());
	p1 = p2;

	BOOST_TEST(Get(p1) == Get(p2));
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));
}

BOOST_AUTO_TEST_CASE(SelfAssignBug)
{
	GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1>());
	const auto * raw = Get(p1);
	p1 = p1;

	BOOST_TEST(p1);
	BOOST_TEST(p1 == p1);
	BOOST_TEST(1U == UseCount(p1));
	BOOST_TEST(Get(p1) == raw);
}

BOOST_AUTO_TEST_CASE(CallMethod)
{
	GLib::Win::ComPtr<ITest1> test(GLib::Win::Make<ImplementsITest1>());
	BOOST_TEST(S_OK == test->Test1Method());
}

BOOST_AUTO_TEST_CASE(QIOk)
{
	GLib::Win::ComPtr<ITest1> p1(GLib::Win::Make<ImplementsITest1AndITest2>());
	GLib::Win::ComPtr<ITest1> p2 = GLib::Win::ComCast<ITest1>(p1);
	BOOST_TEST(2U == UseCount(p1));
	BOOST_TEST(2U == UseCount(p2));

	GLib::Win::ComPtr<IUnknown> pu = GLib::Win::ComCast<IUnknown>(p2);
	BOOST_TEST(Get(pu) == Get(p2));
}

BOOST_AUTO_TEST_CASE(MultipleInheritance)
{
	GLib::Win::ComPtr<ITest1Extended> p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();
	p12->Test1Method();
	p12->Test1ExtendedMethod();

	GLib::Win::ComPtr<ITest1ExtendedAlt> p13 = GLib::Win::ComCast<ITest1ExtendedAlt>(p12);
	p13->Test1Method();
	p13->ITest1ExtendedMethodAlt();
}

BOOST_AUTO_TEST_CASE(ComCast)
{
	GLib::Win::ComPtr<ITest1Extended> p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();

	BOOST_CHECK(true == static_cast<bool>(GLib::Win::ComCast<ITest1>(p12)));

	GLIB_CHECK_EXCEPTION(GLib::Win::ComCast<ITest2>(p12),
		GLib::Win::ComException, "QueryInterface : No such interface supported (80004002)");

	GLIB_CHECK_EXCEPTION(GLib::Win::ComCast<ITest3>(p12),
		GLib::Win::ComException, "QueryInterface : No such interface supported (80004002)");

	BOOST_TEST(true == static_cast<bool>(GLib::Win::ComCast<ITest1Extended>(p12)));
	BOOST_TEST(true == static_cast<bool>(GLib::Win::ComCast<ITest1ExtendedAlt>(p12)));
}

BOOST_AUTO_TEST_CASE(QiMissInDebugOut)
{
	GLib::Win::ComPtr<ITest1Extended> p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();
	GLib::Win::ComPtr<IUnknown> unk;
	HRESULT hr = p12->QueryInterface(__uuidof(IClassFactory), GetAddress(unk));
	BOOST_TEST(hr == E_NOINTERFACE);
}

BOOST_AUTO_TEST_CASE(ComCastOkWithCastOverloadFix)
{
	GLib::Win::ComPtr<ITest1Extended> p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();
	BOOST_TEST(static_cast<bool>(GLib::Win::ComCast<ITest1ExtendedAlt>(p12)));
}

BOOST_AUTO_TEST_SUITE_END()