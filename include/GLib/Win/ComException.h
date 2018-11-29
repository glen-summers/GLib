#pragma once

#include "GLib/Win/WinException.h"
#include "GLib/scope.h"

#include <OleAuto.h>

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			inline std::string FormatErrorInfo(const std::string & message, HRESULT hr)
			{
				bool hasMessage = false;
				std::ostringstream stm;
				stm << message << " : ";
				IErrorInfo* pErrorInfo;
				if (::GetErrorInfo(0, &pErrorInfo) == S_OK)
				{
					BSTR bstr = nullptr;
					pErrorInfo->GetDescription(&bstr);
					SCOPE(_, [=] ()
					{
						::SysFreeString(bstr);
						pErrorInfo->Release();
					});
					hasMessage = !!bstr;
					if (hasMessage)
					{
						stm << Cvt::w2a(bstr);
					}
				}
				if (!hasMessage)
				{
					Util::FormatErrorMessage(stm, hr);
				}
				stm << " (" << std::hex << std::uppercase << hr << ")";
				return stm.str();
			}
		}

		class ComException : public WinException
		{
		public:
			ComException(HRESULT hr, const char * message)
				: WinException(hr, Detail::FormatErrorInfo(message, hr))
			{}
		};
	}
}
