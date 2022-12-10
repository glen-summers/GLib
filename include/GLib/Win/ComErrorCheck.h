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
		inline std::string FormatErrorInfo(std::string_view const message, HRESULT const result)
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
				Util::FormatErrorMessage(stm, result);
			}
			stm << " (" << std::hex << std::uppercase << result << ")";

			return stm.str();
		}

		__declspec(noreturn) inline void Throw(std::string_view const message, HRESULT const result)
		{
			std::string const formattedMessage = FormatErrorInfo(message, result);
#ifdef _DEBUG // || defined(GLIB_DEBUG)
			Debug::Stream() << "ComException : " << formattedMessage << std::endl;
#endif
			throw ComException(formattedMessage, result);
		}
	}

	inline void CheckHr(HRESULT const result, std::string_view const message)
	{
		if (FAILED(result))
		{
			Detail::Throw(message, result);
		}
	}

	inline void WarnHr(HRESULT result, std::string_view message) noexcept
	{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
		if (!SUCCEEDED(result))
		{
			Debug::Stream() << "ComWarning : " << Detail::FormatErrorInfo(message, result) << std::endl;
		}
#else
		static_cast<void>(result);
		static_cast<void>(message);
#endif
	}
}