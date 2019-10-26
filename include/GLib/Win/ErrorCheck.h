#pragma once

#include "GLib/Win/FormatErrorMessage.h"
#include "GLib/Win/WinException.h"

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include "GLib/Win/DebugStream.h"
#endif

#include <sstream>

namespace GLib::Win::Util
{
	namespace Detail
	{
		inline std::string FormatErrorMessage(const char * message, DWORD error, const wchar_t * moduleName = nullptr)
		{
			std::ostringstream stm;
			stm << message << " : ";
			Util::FormatErrorMessage(stm, error, moduleName);
			if (IS_ERROR(error)) // NOLINT(hicpp-signed-bitwise) baad macro
			{
				stm << std::hex;
			}
			stm << " (" << error << ")";
			return stm.str();
		}

		__declspec(noreturn) inline void Throw(const char * message, DWORD result)
		{
			std::string formattedMessage = FormatErrorMessage(message, result);

#ifdef _DEBUG // || defined(GLIB_DEBUG)
			Debug::Stream() << "WinException : " << formattedMessage << std::endl;
#endif
			throw WinException(formattedMessage, result);
		}

		// only allow specific parameters to prevent accidental int -> boolean and misinterpreting win32 results
		// https://stackoverflow.com/questions/175689/can-you-use-keyword-explicit-to-prevent-automatic-conversion-of-method-parameter
		class Checker
		{
		public:
			template<typename T> static void AssertTrue(T value, const char * message, DWORD errorCode)
			{
				(void)value;
				(void)message;
				static_assert(false, "Invalid check parameter, only bool and BOOL allowed");
			}

			static void AssertTrue(bool result, const char * message, DWORD errorCode)
			{
				if (!result)
				{
					Throw(message, errorCode);
				}
			}

			static void AssertTrue(BOOL result, const char * message, DWORD errorCode)
			{
				AssertTrue(result != FALSE, message, errorCode);
			}

			static void WarnAssertTrue(bool result, const char * message, DWORD errorCode) noexcept
			{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
				if (!result)
				{
					Debug::Stream() << "GLib warning: " << Detail::FormatErrorMessage(message, errorCode) << std::endl;
				}
#else
				(void)result;
				(void)message;
				(void)errorCode;
#endif
			}

			static void WarnAssertTrue(BOOL result, const char * message) noexcept
			{
				WarnAssertTrue(result != FALSE, message);
			}
		};
	}

	template <typename T> void AssertTrue(T result, const char * message)
	{
		Detail::Checker::AssertTrue(result, message, ::GetLastError());
	}

	template <typename T> void WarnAssertTrue(T result, const char * message)
	{
		Detail::Checker::WarnAssertTrue(result, message, ::GetLastError());
	}

	inline void AssertSuccess(DWORD errorCode, const char * message)
	{
		Detail::Checker::AssertTrue(errorCode == ERROR_SUCCESS, message, errorCode);
	}

	inline void WarnAssertSuccess(DWORD errorCode, const char * message)
	{
		Detail::Checker::WarnAssertTrue(errorCode == ERROR_SUCCESS, message, errorCode);
	}
}