#include <boost/test/unit_test.hpp>

#ifdef _DEBUG
#define COM_PTR_DEBUG
#define COM_LOG_QI_MISS
#endif

#include "TestInterfaces.h"
#include "TestUtils.h"

#include <GLib/Win/ComException.h>

namespace
{
	template <typename T>
	T * ConstCast(T const * t)
	{
		return const_cast<T *>(t); // NOLINT(cppcoreguidelines-pro-type-const-cast) required
	}

	template <typename T>
	unsigned int UseCount(GLib::Win::ComPtr<T> const & ptr)
	{
		T * p = ConstCast<T>(Get(ptr));
		return !p ? 0 : (p->AddRef(), p->Release());
	}
}

template <typename T>
struct boost::test_tools::tt_detail::print_log_value<GLib::Win::ComPtr<T>>
{
	void operator()(std::ostream & stm, GLib::Win::ComPtr<T> const & item)
	{
		stm << "ptr: " << Get(item) << ", Ref: " << UseCount(item);
	}
};

AUTO_TEST_SUITE(ComPtrTests)

AUTO_TEST_CASE(ComErrorCheckWithErrorInfo)
{
	GLib::Win::ComPtr<ICreateErrorInfo> p;
	GLib::Win::CheckHr(CreateErrorInfo(GetAddress(p).Raw()), "CreateErrorInfo");
	auto ei = GLib::Win::ComCast<IErrorInfo>(p);
	p->SetDescription(ConstCast<WCHAR>(L"hello"));
	GLib::Win::CheckHr(SetErrorInfo(0, Get(ei)), "SetErrorInfo");

	GLIB_CHECK_EXCEPTION(GLib::Win::CheckHr(E_OUTOFMEMORY, "fail"), GLib::Win::ComException, "fail : hello (8007000E)");
}

AUTO_TEST_CASE(ComErrorCheck)
{
	SetErrorInfo(0, nullptr);
	GLIB_CHECK_EXCEPTION(GLib::Win::CheckHr(E_FAIL, "test E_FAIL"), GLib::Win::ComException, "test E_FAIL : Unspecified error (80004005)");
}

AUTO_TEST_CASE(ComErrorCheck2)
{
	SetErrorInfo(0, nullptr);
	GLIB_CHECK_EXCEPTION(GLib::Win::CheckHr(E_UNEXPECTED, "test E_UNEXPECTED"), GLib::Win::ComException,
											 "test E_UNEXPECTED : Catastrophic failure (8000FFFF)");
}

AUTO_TEST_CASE(UninitialisedComPtrHasZeroUseCount)
{
	GLib::Win::ComPtr<ITest1> p;
	TEST(0U == UseCount(p));
	TEST(false == static_cast<bool>(p));
	TEST(nullptr == p);
	TEST(p == nullptr);
	TEST(nullptr == Get(p));
}

AUTO_TEST_CASE(NullInitialisedComPtrHasZeroUseCount)
{
	GLib::Win::ComPtr<ITest1> p1(nullptr);
	TEST(0U == UseCount(p1));
	TEST(false == static_cast<bool>(p1));
	TEST(nullptr == Get(p1));

	GLib::Win::ComPtr<ITest1> p2 = {};
	TEST(0U == UseCount(p2));
	TEST(false == static_cast<bool>(p2));
	TEST(nullptr == Get(p2));
}

AUTO_TEST_CASE(InitialisedComPtrHasOneUseCount)
{
	GLib::Win::ComPtr p1(GLib::Win::Make<ImplementsITest1>());

	TEST(1U == UseCount(p1));
	TEST(nullptr != p1);
	TEST(p1 != nullptr);
	TEST(nullptr != Get(p1));
}

AUTO_TEST_CASE(CtorFromSameType)
{
	GLib::Win::ComPtr const p1(GLib::Win::Make<ImplementsITest1>());
	TEST(1U == UseCount(p1));

	GLib::Win::ComPtr p2(p1);
	TEST(2U == UseCount(p1));
	TEST(2U == UseCount(p2));

	GLib::Win::ComPtr const p2m(std::move(p2));
	TEST(2U == UseCount(p1));
#pragma warning(push)
#pragma warning(disable : 26800)
	TEST(0U == UseCount(p2)); // moved object is ok
#pragma warning(pop)
	TEST(2U == UseCount(p2m));
}

AUTO_TEST_CASE(CtorFromRawValue)
{
	GLib::Win::ComPtr p1(GLib::Win::Make<ImplementsITest1>());
	GLib::Win::ComPtr<ITest1> const p2(Get(p1));
	TEST(2U == UseCount(p1));
	TEST(2U == UseCount(p2));
}

AUTO_TEST_CASE(CtorFromOtherType)
{
	GLib::Win::ComPtr const p1(GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>());
	TEST(1U == UseCount(p1));

	GLib::Win::ComPtr<ITest1> p2(p1);
	TEST(2U == UseCount(p1));
	TEST(2U == UseCount(p2));

	GLib::Win::ComPtr const p2m(std::move(p2));
	TEST(2U == UseCount(p1));
#pragma warning(push)
#pragma warning(disable : 26800)
	TEST(0U == UseCount(p2)); // moved object is ok
#pragma warning(pop)
	TEST(2U == UseCount(p2m));
}

AUTO_TEST_CASE(AssignmentSameType)
{
	GLib::Win::ComPtr<ITest1> p1;
	GLib::Win::ComPtr<ITest1> p2;
	GLib::Win::ComPtr<ITest1> p3;
	p1 = GLib::Win::Make<ImplementsITest1>();
	TEST(1U == UseCount(p1));
	TEST(0U == UseCount(p2));
	TEST(0U == UseCount(p3));

	p2 = p1;
	TEST(2U == UseCount(p1));
	TEST(2U == UseCount(p2));
	TEST(0U == UseCount(p3));

	p3 = std::move(p1);
#pragma warning(push)
#pragma warning(disable : 26800)
	TEST(0U == UseCount(p1)); // moved object is ok
#pragma warning(pop)
	TEST(2U == UseCount(p2));
	TEST(2U == UseCount(p3));
}

AUTO_TEST_CASE(AssignmentOtherType)
{
	GLib::Win::ComPtr p1(GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>());
	GLib::Win::ComPtr<ITest1> p2;
	GLib::Win::ComPtr<ITest1> p3;
	TEST(1U == UseCount(p1));
	TEST(0U == UseCount(p2));
	TEST(0U == UseCount(p3));

	p2 = p1;
	TEST(2U == UseCount(p1));
	TEST(2U == UseCount(p2));
	TEST(0U == UseCount(p3));

	p3 = std::move(p1);
#pragma warning(push)
#pragma warning(disable : 26800)
	TEST(0U == UseCount(p1)); // moved object is ok
#pragma warning(pop)

	TEST(2U == UseCount(p2));
	TEST(2U == UseCount(p3));
}

AUTO_TEST_CASE(SelfAssign)
{
	GLib::Win::ComPtr p1(GLib::Win::Make<ImplementsITest1>());
	static_cast<void>(p1);
	GLib::Win::ComPtr p2(GLib::Win::Make<ImplementsITest1>());
	p1 = p2;

	TEST(Get(p1) == Get(p2));
	TEST(2U == UseCount(p1));
	TEST(2U == UseCount(p2));
}

AUTO_TEST_CASE(SelfAssignBug)
{
	GLib::Win::ComPtr p1(GLib::Win::Make<ImplementsITest1>());
	auto const * raw = Get(p1);
	p1 = p1; // NOLINT(clang-diagnostic-self-assign-overloaded) test

	TEST(p1);
	TEST(p1 == p1);
	TEST(1U == UseCount(p1));
	TEST(Get(p1) == raw);
}

AUTO_TEST_CASE(CallMethod)
{
	GLib::Win::ComPtr const test(GLib::Win::Make<ImplementsITest1>());
	TEST(S_OK == test->Test1Method());
}

AUTO_TEST_CASE(QiOk)
{
	GLib::Win::ComPtr const p1(GLib::Win::Make<ImplementsITest1AndITest2>());
	GLib::Win::ComPtr<ITest1> p2 = GLib::Win::ComCast<ITest1>(p1);
	TEST(2U == UseCount(p1));
	TEST(2U == UseCount(p2));

	GLib::Win::ComPtr<IUnknown> pu = GLib::Win::ComCast<IUnknown>(p2);
	TEST(Get(pu) == Get(p2));
}

AUTO_TEST_CASE(MultipleInheritance)
{
	GLib::Win::ComPtr<ITest1Extended> const p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();
	p12->Test1Method();
	p12->Test1ExtendedMethod();

	GLib::Win::ComPtr<ITest1ExtendedAlt> const p13 = GLib::Win::ComCast<ITest1ExtendedAlt>(p12);
	p13->Test1Method();
	p13->ITest1ExtendedMethodAlt();
}

AUTO_TEST_CASE(ComCast)
{
	GLib::Win::ComPtr<ITest1Extended> const p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();

	CHECK(true == static_cast<bool>(GLib::Win::ComCast<ITest1>(p12)));

	GLIB_CHECK_EXCEPTION(GLib::Win::ComCast<ITest2>(p12), GLib::Win::ComException, "QueryInterface : No such interface supported (80004002)");

	GLIB_CHECK_EXCEPTION(GLib::Win::ComCast<ITest3>(p12), GLib::Win::ComException, "QueryInterface : No such interface supported (80004002)");

	TEST(true == static_cast<bool>(GLib::Win::ComCast<ITest1Extended>(p12)));
	TEST(true == static_cast<bool>(GLib::Win::ComCast<ITest1ExtendedAlt>(p12)));
}

AUTO_TEST_CASE(QiMissInDebugOut)
{
	GLib::Win::ComPtr<ITest1Extended> const p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();
	GLib::Win::ComPtr<IUnknown> unk;
	HRESULT hr = p12->QueryInterface(GLib::Win::GetUuId<IClassFactory>(), GetAddress(unk).Void());
	TEST(hr == E_NOINTERFACE);
}

AUTO_TEST_CASE(ComCastOkWithCastOverloadFix)
{
	GLib::Win::ComPtr<ITest1Extended> const p12 = GLib::Win::Make<ImplementsITest1ExtendedAndITest1ExtendedAlt>();
	TEST(static_cast<bool>(GLib::Win::ComCast<ITest1ExtendedAlt>(p12)));
}

AUTO_TEST_SUITE_END()