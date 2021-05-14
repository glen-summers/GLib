#pragma once

#include <GLib/Win/ComException.h>
#include <GLib/Win/FormatErrorMessage.h>

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include <GLib/Win/DebugStream.h>
#endif

#include <GLib/scope.h>

#include <sstream>

#include <OleAuto.h>

namespace GLib::Win
{
	namespace Detail
	{
		inline std::string FormatErrorInfo(const char * message, HRESULT hr)
		{
			bool hasMessage = false;
			std::ostringstream stm;
			stm << message << " : ";
			IErrorInfo * pErrorInfo = nullptr;
			if (::GetErrorInfo(0, &pErrorInfo) == S_OK)
			{
				BSTR description = nullptr;
				pErrorInfo->GetDescription(&description);
				SCOPE(_,
							[=]() noexcept
							{
								::SysFreeString(description);
								pErrorInfo->Release();
							});
				hasMessage = description != nullptr;
				if (hasMessage)
				{
					stm << Cvt::w2a(description);
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
			throw ComException(formattedMessage, hr);
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
		(void) hr;
		(void) message;
#endif
	}
}