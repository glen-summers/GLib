#pragma once

#include <GLib/Win/SimpleCom.h>

// NOLINTBEGIN
MIDL_INTERFACE("26FA5481-DA4F-45A8-8F83-F099E951C6E2")
ITest1 : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Test1Method() = 0;
};

MIDL_INTERFACE("4FF4868D-5310-488A-9569-327CFC292BB5")
ITest2 : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Test2Method() = 0;
};

MIDL_INTERFACE("D550B0D8-2EAA-4D11-9EF5-9412FB17A013")
ITest3 : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE Test3Method() = 0;
};

MIDL_INTERFACE("{2AC5AAB0-31A2-4B94-89C3-86DDA1A45E54}")
ITest1Extended : ITest1
{
	virtual HRESULT STDMETHODCALLTYPE Test1ExtendedMethod() = 0;
};

MIDL_INTERFACE("91C5058A-B2EE-424A-AE03-C2EE7320A79D")
ITest1ExtendedAlt : ITest1
{
	virtual HRESULT STDMETHODCALLTYPE ITest1ExtendedMethodAlt() = 0;
};
// NOLINTEND

template <typename T>
struct DeleteCounter
{
	DeleteCounter() = default;
	DeleteCounter(DeleteCounter const &) = delete;
	DeleteCounter(DeleteCounter &&) = delete;
	DeleteCounter & operator=(DeleteCounter const &) = delete;
	DeleteCounter & operator=(DeleteCounter &&) = delete;

	~DeleteCounter()
	{
		DeleteCount(1);
	}

	static void Reset()
	{
		DeleteCount(0);
	}

	static int DeleteCount(int const add = 0)
	{
		static int deleteCount = 0;
		int const d = deleteCount += add;
		if (add == 0)
		{
			deleteCount = 0;
		}
		return d;
	}
};

class ImplementsITest1 final : public GLib::Win::Unknown<ImplementsITest1, ITest1>
{
	DeleteCounter<ImplementsITest1> counter;

public:
	ImplementsITest1() = default;
	ImplementsITest1(ImplementsITest1 const &) = delete;
	ImplementsITest1(ImplementsITest1 &&) = delete;
	ImplementsITest1 & operator=(ImplementsITest1 const &) = delete;
	ImplementsITest1 & operator=(ImplementsITest1 &&) = delete;
	~ImplementsITest1() override = default;

	HRESULT STDMETHODCALLTYPE Test1Method() override
	{
		return S_OK;
	}
};

class ImplementsITest1AndITest2 final : public GLib::Win::Unknown<ImplementsITest1AndITest2, ITest1, ITest2>
{
	DeleteCounter<ImplementsITest1AndITest2> counter;

public:
	ImplementsITest1AndITest2() = default;
	ImplementsITest1AndITest2(ImplementsITest1AndITest2 const &) = delete;
	ImplementsITest1AndITest2(ImplementsITest1AndITest2 &&) = delete;
	ImplementsITest1AndITest2 & operator=(ImplementsITest1AndITest2 const &) = delete;
	ImplementsITest1AndITest2 & operator=(ImplementsITest1AndITest2 &&) = delete;
	~ImplementsITest1AndITest2() override = default;

	HRESULT STDMETHODCALLTYPE Test1Method() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Test2Method() override
	{
		return S_OK;
	}
};

class ImplementsITest1ExtendedAndITest1ExtendedAlt;

class ImplementsITest1ExtendedAndITest1ExtendedAlt final
	: public GLib::Win::Unknown<ImplementsITest1ExtendedAndITest1ExtendedAlt, ITest1Extended, ITest1, ITest1ExtendedAlt>
{
	DeleteCounter<ImplementsITest1ExtendedAndITest1ExtendedAlt> counter;

public:
	ImplementsITest1ExtendedAndITest1ExtendedAlt() = default;
	ImplementsITest1ExtendedAndITest1ExtendedAlt(ImplementsITest1ExtendedAndITest1ExtendedAlt const &) = delete;
	ImplementsITest1ExtendedAndITest1ExtendedAlt(ImplementsITest1ExtendedAndITest1ExtendedAlt &&) = delete;
	ImplementsITest1ExtendedAndITest1ExtendedAlt & operator=(ImplementsITest1ExtendedAndITest1ExtendedAlt const &) = delete;
	ImplementsITest1ExtendedAndITest1ExtendedAlt & operator=(ImplementsITest1ExtendedAndITest1ExtendedAlt &&) = delete;
	~ImplementsITest1ExtendedAndITest1ExtendedAlt() override = default;

	HRESULT STDMETHODCALLTYPE Test1Method() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE Test1ExtendedMethod() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE ITest1ExtendedMethodAlt() override
	{
		return S_OK;
	}
};

namespace GLib::Win::Detail
{
	template <>
	inline ITest1 * Cast<ITest1, ImplementsITest1ExtendedAndITest1ExtendedAlt>(ImplementsITest1ExtendedAndITest1ExtendedAlt * t)
	{
		return static_cast<ITest1Extended *>(t);
	}
}
