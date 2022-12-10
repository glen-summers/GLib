#pragma once

#include <Windows.h>

#include <stdexcept>

namespace GLib::Win
{
	class WinException : public std::runtime_error
	{
		ULONG const errorCode;
		HRESULT const hResult;

	public:
		[[nodiscard]] ULONG ErrorCode() const
		{
			return errorCode;
		}

		[[nodiscard]] HRESULT HResult() const
		{
			return hResult;
		}

		WinException(std::string const & message, ULONG const dwErr)
			: runtime_error(message)
			, errorCode(dwErr)
			, hResult(HRESULT_FROM_WIN32(dwErr))
		{}

	protected:
		WinException(std::string const & message, HRESULT const result)
			: runtime_error(message)
			, errorCode(static_cast<unsigned int>(result))
			, hResult(result)
		{}
	};
}