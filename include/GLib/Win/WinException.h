#pragma once

#include <Windows.h>

#include <stdexcept>

namespace GLib::Win
{
	class WinException : public std::runtime_error
	{
		DWORD const errorCode;
		HRESULT const hResult;

	public:
		DWORD ErrorCode() const
		{
			return errorCode;
		}

		HRESULT HResult() const
		{
			return hResult;
		}

		WinException(const std::string & message, DWORD dwErr)
			: runtime_error(message)
			, errorCode(dwErr)
			, hResult(HRESULT_FROM_WIN32(dwErr))
		{}

	protected:
		WinException(const std::string & message, HRESULT hr)
			: runtime_error(message)
			, errorCode(static_cast<unsigned int>(hr))
			, hResult(hr)
		{}
	};
}