#pragma once

#include "GLib/Win/ComPtr.h"
#include "GLib/Win/SimpleCom.h"

MIDL_INTERFACE("26FA5481-DA4F-45A8-8F83-F099E951C6E2")
ITest1 : IUnknown
{
	virtual HRESULT STDMETHODCALLTYPE TestMethod1() = 0;
};

MIDL_INTERFACE("4FF4868D-5310-488A-9569-327CFC292BB5")
ITest2 : IUnknown // derive from ITest breaks Make()
{
	virtual HRESULT STDMETHODCALLTYPE TestMethod2() = 0;
};

class ImplementsITest1 final : public GLib::Win::Unknown<ITest1>
{
public:
	static int DeleteCount(int add = 0)
	{
		static int deleteCount = 0;
		int d = deleteCount += add;
		if (add == 0)
		{
			deleteCount = 0;
		}
		return d;
	}

protected:
	~ImplementsITest1()
	{
		DeleteCount(1);
	}

	HRESULT STDMETHODCALLTYPE TestMethod1() override
	{
		return S_OK;
	}
};

class ImplementsITest2AndITest1 final : public GLib::Win::Unknown<ITest2, ITest1>
{
public:
	static int DeleteCount(int add = 0)
	{
		static int deleteCount = 0;
		int d = deleteCount += add;
		if (add == 0)
		{
			deleteCount = 0;
		}
		return d;
	}

protected:
	~ImplementsITest2AndITest1()
	{
		DeleteCount(1);
	}

	HRESULT STDMETHODCALLTYPE TestMethod1() override
	{
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE TestMethod2() override
	{
		return S_OK;
	}
};
