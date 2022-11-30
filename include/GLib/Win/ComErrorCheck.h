#pragma once

#include <GLib/Win/Bstr.h>
#include <GLib/Win/ComException.h>
#include <GLib/Win/ComPtr.h>
#include <GLib/Win/FormatErrorMessage.h>

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include <GLib/Win/DebugStream.h>
#endif

#include <sstream>

#include <OleAuto.h>

namespace GLib::Win
{
	namespace Detail
	{
		inline std::string FormatErrorInfo(std::string_view const message, HRESULT const hr)
		{
			std::ostringstream stm;
			stm << message << " : ";
			ComPtr<IErrorInfo> errorInfo;
			if (GetErrorInfo(0, GetAddress(errorInfo).Raw()) == S_OK)
			{
				Bstr description;
				errorInfo->GetDescription(GetAddress(description).Raw());
				if (description.HasValue())
				{
					stm << description.Value();
				}
			}
			else
			{
				Util::FormatErrorMessage(stm, hr);
			}
			stm << " (" << std::hex << std::uppercase << hr << ")";

			return stm.str();
		}

		__declspec(noreturn) inline void Throw(std::string_view const message, HRESULT const hr)
		{
			std::string const formattedMessage = FormatErrorInfo(message, hr);
#ifdef _DEBUG // || defined(GLIB_DEBUG)
			Debug::Stream() << "ComException : " << formattedMessage << std::endl;
#endif
			throw ComException(formattedMessage, hr);
		}
	}

	inline void CheckHr(HRESULT const hr, std::string_view const message)
	{
		if (FAILED(hr))
		{
			Detail::Throw(message, hr);
		}
	}

	inline void WarnHr(HRESULT hr, std::string_view message) noexcept
	{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
		bool const result = SUCCEEDED(hr);
		if (!result)
		{
			Debug::Stream() << "ComWarning : " << Detail::FormatErrorInfo(message, hr) << std::endl;
		}
#else
		static_cast<void>(hr);
		static_cast<void>(message);
#endif
	}
}