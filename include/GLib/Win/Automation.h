#pragma once

#include <GLib/Win/Bstr.h>
#include <GLib/Win/ComPtr.h>
#include <GLib/Win/Handle.h>

#include <UIAutomation.h>

namespace GLib::Win
{
	class Element
	{
		ComPtr<IUIAutomationElement> element;

	public:
		explicit Element(ComPtr<IUIAutomationElement> element)
			: element(std::move(element))
		{}

		template <typename T>
		ComPtr<T> GetCurrentPattern(long const id)
		{
			ComPtr<IUnknown> pattern;
			CheckHr(element->GetCurrentPattern(id, GetAddress(pattern).Raw()), "GetCurrentPattern");
			if (!pattern)
			{
				throw std::exception("Pattern not implemented");
			}
			return ComCast<T>(pattern);
		}

		[[nodiscard]] std::string CurrentName() const
		{
			Bstr bstr;
			CheckHr(element->get_CurrentName(GetAddress(bstr).Raw()), "CurrentName");
			return bstr.Value();
		}

		[[nodiscard]] std::string CurrentClassName() const
		{
			Bstr bstr;
			CheckHr(element->get_CurrentClassName(GetAddress(bstr).Raw()), "CurrentClassName");
			return bstr.Value();
		}
	};

	class Automation
	{
		ComPtr<IUIAutomation> automation;

	public:
		Automation()
		{
			CheckHr(CoCreateInstance(GetUuId<CUIAutomation>(), nullptr, ComPtrDetail::ContextAll, GetUuId<IUIAutomation>(), GetAddress(automation).Void()),
							"CoCreateInstance");
		}

		Element ElementFromHandle(WindowHandleBase * const hWnd) const
		{
			ComPtr<IUIAutomationElement> element;
			CheckHr(automation->ElementFromHandle(hWnd, GetAddress(element).Raw()), "ElementFromHandle");
			return Element {std::move(element)};
		}
	};
}