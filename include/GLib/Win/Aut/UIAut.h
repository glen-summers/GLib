#pragma once

#include <GLib/Win/ComPtr.h>
#include <GLib/Win/Bstr.h>

#include <uiautomation.h>

namespace GLib::Win::Aut
{
	class UIElement
	{
		ComPtr<IUIAutomationElement> element;

	public:
		UIElement(const ComPtr<IUIAutomationElement> & element) : element(element)
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

	class UIAut
	{
		ComPtr<IUIAutomation> automation;

	public:
		UIAut()
		{
			CheckHr(::CoCreateInstance(__uuidof(CUIAutomation), nullptr, ComPtrDetail::ContextAll, __uuidof(IUIAutomation),
				GetAddress(automation)), "CoCreateInstance");
		}

		UIElement ElementFromHandle(HWND hWnd) const
		{
			ComPtr<IUIAutomationElement> element;
			CheckHr(automation->ElementFromHandle(hWnd, GetAddress(element)), "ElementFromHandle");
			// null check?
			return element;
		}
	};
}