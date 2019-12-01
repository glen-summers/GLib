#pragma once

#include <GLib/Win/FormatErrorMessage.h>
#include <GLib/Win/WinException.h>

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include <GLib/Win/DebugStream.h>
#endif

#include <sstream>

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

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
			template<typename T> static bool AssertTrue(T value, const char * message, DWORD errorCode)
			{
				static_assert(false, "Invalid check parameter, only bool and BOOL allowed");
			}

			template<typename T> static bool WarnAssertTrue(T value, const char * message, DWORD errorCode)
			{
				static_assert(false, "Invalid check parameter, only bool and BOOL allowed");
			}

			template<typename T> static bool AssertSuccess(T value, const char * message)
			{
				static_assert(false, "Invalid check parameter, only DWORD allowed");
			}

			template<typename T> static bool WarnAssertSuccess(T value, const char * message)
			{
				static_assert(false, "Invalid check parameter, only DWORD and LSTATUS allowed");
			}

			static bool AssertTrue(bool result, const char * message, DWORD errorCode)
			{
				if (!result)
				{
					Throw(message, errorCode);
				}
				return result;
			}

			static bool AssertTrue(BOOL result, const char * message, DWORD errorCode)
			{
				return AssertTrue(result != FALSE, message, errorCode);
			}

			static bool WarnAssertTrue(bool result, const char * message, DWORD errorCode) noexcept
			{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
				if (!result)
				{
					Debug::Stream() << "GLib warning: " << Detail::FormatErrorMessage(message, errorCode) << std::endl;
				}
#else
				(void)message; (void)errorCode;
#endif
				return result;
			}

			static bool WarnAssertTrue(BOOL result, const char * message, DWORD errorCode) noexcept
			{
				return WarnAssertTrue(result != FALSE, message, errorCode);
			}

			static bool AssertSuccess(DWORD result, const char * message)
			{
				return AssertTrue(result == ERROR_SUCCESS, message, result);
			}

			static bool WarnAssertSuccess(DWORD result, const char * message) noexcept
			{
				return WarnAssertTrue(result == ERROR_SUCCESS, message, result);
			}

			static bool AssertSuccess(LSTATUS result, const char * message)
			{
				return AssertTrue(result == ERROR_SUCCESS, message, static_cast<DWORD>(result));
			}

			static bool WarnAssertSuccess(LSTATUS result, const char * message) noexcept
			{
				return WarnAssertTrue(result == ERROR_SUCCESS, message, static_cast<DWORD>(result));
			}
		};
	}

	template <typename T> bool AssertTrue(T result, const char * message)
	{
		return Detail::Checker::AssertTrue(result, message, ::GetLastError());
	}

	template <typename T> bool WarnAssertTrue(T result, const char * message)
	{
		return Detail::Checker::WarnAssertTrue(result, message, ::GetLastError());
	}

	template <typename T> bool AssertSuccess(T errorCode, const char * message)
	{
		return Detail::Checker::AssertSuccess(errorCode, message);
	}

	template <typename T> bool WarnAssertSuccess(T errorCode, const char * message)
	{
		return Detail::Checker::WarnAssertSuccess(errorCode, message);
	}
}