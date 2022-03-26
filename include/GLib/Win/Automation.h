#pragma once

#include <GLib/Win/Bstr.h>
#include <GLib/Win/ComPtr.h>

#include <UIAutomation.h>

namespace GLib::Win
{
	class Element
	{
		ComPtr<IUIAutomationElement> element;

	public:
		Element(ComPtr<IUIAutomationElement> element)
			: element(std::move(element))
		{}

		template <typename T>
		ComPtr<T> GetCurrentPattern(long id)
		{
			ComPtr<IUnknown> pattern;
			CheckHr(element->GetCurrentPattern(id, GetAddress(pattern)), "GetCurrentPattern");
			if (!pattern)
			{
				throw std::exception("Pattern not implemented");
			}
			return ComCast<T>(pattern);
		}

		std::string CurrentName() const
		{
			Bstr bstr;
			CheckHr(element->get_CurrentName(GetAddress(bstr)), "CurrentName");
			return bstr.Value();
		}

		std::string CurrentClassName() const
		{
			Bstr bstr;
			CheckHr(element->get_CurrentClassName(GetAddress(bstr)), "CurrentClassName");
			return bstr.Value();
		}
	};

	class Automation
	{
		ComPtr<IUIAutomation> automation;

	public:
		Automation()
		{
			CheckHr(CoCreateInstance(__uuidof(CUIAutomation), nullptr, ComPtrDetail::ContextAll, __uuidof(IUIAutomation), GetAddress(automation)),
							"CoCreateInstance");
		}

		Element ElementFromHandle(HWND hWnd) const
		{
			ComPtr<IUIAutomationElement> element;
			CheckHr(automation->ElementFromHandle(hWnd, GetAddress(element)), "ElementFromHandle");
			return std::move(element);
		}
	};
}