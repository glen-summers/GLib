#pragma once

#include "GLib/Win/FormatErrorMessage.h"

#include <sstream>

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			inline std::string FormatErrorMessage(const char * message, DWORD error, const wchar_t * moduleName = nullptr)
			{
				std::ostringstream stm;
				stm << message << " : ";
				Util::FormatErrorMessage(stm, error, moduleName);
				if (error >= 0x80000000) stm << std::hex;
				stm << " (" << error << ")";
				return stm.str();
			}
		}

		class WinException : public std::runtime_error
		{
			unsigned int const errorCode;
			HRESULT const hResult;

		public:
			unsigned int ErrorCode() const
			{
				return errorCode;
			}

			HRESULT HResult() const
			{
				return hResult;
			}

			WinException(const char * message, DWORD dwErr, const wchar_t * module = nullptr)
				: runtime_error(Detail::FormatErrorMessage(message, dwErr, module))
				, errorCode(dwErr)
				, hResult(HRESULT_FROM_WIN32(dwErr))
			{}

		protected:
			WinException(HRESULT hr, const std::string & message)
				: runtime_error(message)
				, errorCode(static_cast<unsigned int>(hr))
				, hResult(hr)
			{}
		};
	}
}