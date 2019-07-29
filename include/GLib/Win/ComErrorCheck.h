#pragma once

#include "GLib/Win/ComException.h"
#include "GLib/scope.h"

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include "GLib/Win/DebugStream.h"
#endif

#include <OleAuto.h>

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			inline std::string FormatErrorInfo(const char * message, HRESULT hr)
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
					hasMessage = bstr != nullptr;
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

			__declspec(noreturn) inline void Throw(const char * message, HRESULT hr)
			{
				std::string formattedMessage = FormatErrorInfo(message, hr);
#ifdef _DEBUG // || defined(GLIB_DEBUG)
				Debug::Stream() << "ComException : " << formattedMessage << std::endl;
#endif
				throw ComException(move(formattedMessage), hr);
			}
		}

		inline void CheckHr(HRESULT hr, const char * message)
		{
			if (FAILED(hr))
			{
				Detail::Throw(message, hr);
			}
		}

		inline void WarnHr(HRESULT hr, const char * message) noexcept
		{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
			bool result = SUCCEEDED(hr);
			if (!result)
			{
				Debug::Stream() << "ComWarning : " << Detail::FormatErrorInfo(message, hr) << std::endl;
			}
#else
				UNREFERENCED_PARAMETER(hr);
				UNREFERENCED_PARAMETER(message);
#endif
		}
	}
}
