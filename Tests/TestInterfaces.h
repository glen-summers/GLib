#pragma once

#include "GLib/Win/SimpleCom.h"

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

	HRESULT STDMETHODCALLTYPE Test1Method() override
	{
		return S_OK;
	}

public:
	void ConcreteMethod() const { (void)*this; }
};

class ImplementsITest1AndITest2 final : public GLib::Win::Unknown<ITest1, ITest2>, public DeleteCounter<ImplementsITest1AndITest2>
{
	GLIB_COM_RULE_OF_FIVE(ImplementsITest1AndITest2)

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
	: public GLib::Win::Unknown2<ImplementsITest1ExtendedAndITest1ExtendedAlt,
				GLib::Win::TypeList<ITest1Extended, ITest1ExtendedAlt>,
				GLib::Win::TypeList<ITest1, ITest1Extended, ITest1ExtendedAlt>>
	, public DeleteCounter<ImplementsITest1ExtendedAndITest1ExtendedAlt>
{
	GLIB_COM_RULE_OF_FIVE(ImplementsITest1ExtendedAndITest1ExtendedAlt)

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

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			template <>
			inline ITest1 * Cast<ITest1, ImplementsITest1ExtendedAndITest1ExtendedAlt>
				(ImplementsITest1ExtendedAndITest1ExtendedAlt * t)
			{
				return static_cast<ITest1*>(static_cast<ITest1Extended*>(t));
			}
		}
	}
}
