#pragma once

#include "GLib/Win/FormatErrorMessage.h"

#include <sstream>

namespace GLib::Win
{
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

		WinException(std::string message, DWORD dwErr)
			: runtime_error(move(message))
			, errorCode(dwErr)
			, hResult(HRESULT_FROM_WIN32(dwErr))
		{}

	protected:
		WinException(std::string message, HRESULT hr)
			: runtime_error(move(message))
			, errorCode(static_cast<unsigned int>(hr))
			, hResult(hr)
		{}
	};
}