#pragma once

#include "GLib/Win/SimpleCom.h"

MIDL_INTERFACE("26FA5481-DA4F-45A8-8F83-F099E951C6E2")
ITest1 : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE TestMethod1() = 0;
};

MIDL_INTERFACE("4FF4868D-5310-488A-9569-327CFC292BB5")
ITest2 : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE TestMethod2() = 0;
};

MIDL_INTERFACE("D550B0D8-2EAA-4D11-9EF5-9412FB17A013")
ITest3 : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE TestMethod3() = 0;
};

MIDL_INTERFACE("{2AC5AAB0-31A2-4B94-89C3-86DDA1A45E54}")
ITest12 : ITest1
{
	virtual HRESULT STDMETHODCALLTYPE TestMethod12() = 0;
};

MIDL_INTERFACE("91C5058A-B2EE-424A-AE03-C2EE7320A79D")
ITest13 : ITest1
{
	virtual HRESULT STDMETHODCALLTYPE TestMethod13() = 0;
};

template <typename T>
struct DeleteCounter
{
	DeleteCounter() = default;
	DeleteCounter(const DeleteCounter& other) = default;
	DeleteCounter(DeleteCounter&& other) noexcept = default;
	DeleteCounter& operator=(const DeleteCounter& other) = default;
	DeleteCounter& operator=(DeleteCounter&& other) noexcept = default;

	static void Reset()
	{
		DeleteCount(0);
	}

	~DeleteCounter()
	{
		DeleteCount(1);
	}

	static int DeleteCount(int add = 0)
	{
		static int deleteCount = 0;
		const int d = deleteCount += add;
		if (add == 0)
		{
			deleteCount = 0;
		}
		return d;
	}
};

class ImplementsITest1 final : public GLib::Win::Unknown<ITest1>, public DeleteCounter<ImplementsITest1>
{
	GLIB_COM_RULE_OF_FIVE(ImplementsITest1)

	HRESULT STDMETHODCALLTYPE TestMethod1() override
	{
		return S_OK;
	}

public:
	void ConcreteMethod() const { (void)*this; }
};

class ImplementsITest1AndITest2 final : public GLib::Win::Unknown<ITest1, ITest2>, public DeleteCounter<ImplementsITest1AndITest2>
{
	GLIB_COM_RULE_OF_FIVE(ImplementsITest1AndITest2)

	HRESULT STDMETHODCALLTYPE TestMethod1() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE TestMethod2() override
	{
		return S_OK;
	}
};

// broken: Unknown<ITest12, ITest13> does not implement sub interfaces
class ImplementsITest12AndITest13 final : public GLib::Win::Unknown<ITest12, ITest13>, public DeleteCounter<ImplementsITest12AndITest13>
{
	GLIB_COM_RULE_OF_FIVE(ImplementsITest12AndITest13)

	HRESULT STDMETHODCALLTYPE TestMethod1() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE TestMethod12() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE TestMethod13() override
	{
		return S_OK;
	}
};
