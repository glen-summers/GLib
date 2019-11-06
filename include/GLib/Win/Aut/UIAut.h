#pragma once

#include <GLib/Win/ComPtr.h>

#include <uiautomation.h>

namespace GLib::Win::Aut
{
	namespace Detail
	{
		class TransferBstr // move
		{
			BSTR bstr;
			std::string & ref;

		public:
			TransferBstr(std::string & ref)
				: bstr()
				, ref(ref)
			{}

			TransferBstr(const TransferBstr & ref) = delete;
			TransferBstr(TransferBstr && ref) = delete;
			const TransferBstr& operator=(const TransferBstr &) = delete;
			const TransferBstr& operator=(TransferBstr &&) = delete;
			~TransferBstr()
			{
				ref = bstr ? Cvt::w2a(bstr) : "";
				::SysFreeString(bstr);
			}

			BSTR * operator&()
			{
				return &bstr;
			}
		};
	}

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
			std::string value;
			CheckHr(element->get_CurrentName(&Detail::TransferBstr(value)), "CurrentName");
			return value;
		}

		std::string CurrentClassName() const
		{
			std::string value;
			CheckHr(element->get_CurrentClassName(&Detail::TransferBstr(value)), "CurrentClassName");
			return value;
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