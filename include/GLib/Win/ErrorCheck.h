#pragma once

#include <GLib/Win/FormatErrorMessage.h>
#include <GLib/Win/WinException.h>

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include <GLib/Win/DebugStream.h>
#endif

#include <sstream>

EXTERN_C IMAGE_DOS_HEADER
	__ImageBase; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,cppcoreguidelines-avoid-non-const-global-variables) required

namespace GLib::Win::Util
{
	namespace Detail
	{
		inline bool IsError(DWORD value)
		{
			return IS_ERROR(value); // NOLINT(hicpp-signed-bitwise) bad macro
		}

		inline std::string FormatErrorMessage(std::string_view message, DWORD error, const wchar_t * moduleName = {})
		{
			std::ostringstream stm;
			stm << message << " : ";
			Util::FormatErrorMessage(stm, error, moduleName);
			if (IsError(error))
			{
				stm << std::hex;
			}
			stm << " (" << error << ")";
			return stm.str();
		}

		__declspec(noreturn) inline void Throw(std::string_view message, DWORD result)
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
			template <typename T>
			static bool AssertTrue(T value, std::string_view message, DWORD errorCode) = delete;

			template <typename T>
			static bool WarnAssertTrue(T value, std::string_view message, DWORD errorCode) = delete;

			template <typename T>
			static bool AssertSuccess(T value, std::string_view message) = delete;

			template <typename T>
			static bool WarnAssertSuccess(T value, std::string_view message) = delete;

			static bool AssertTrue(bool result, std::string_view message, DWORD errorCode)
			{
				if (!result)
				{
					Throw(message, errorCode);
				}
				return result;
			}

			static bool AssertTrue(BOOL result, std::string_view message, DWORD errorCode)
			{
				return AssertTrue(result != FALSE, message, errorCode);
			}

			static bool WarnAssertTrue(bool result, std::string_view message, DWORD errorCode) noexcept
			{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
				if (!result)
				{
					Debug::Stream() << "GLib warning: " << Detail::FormatErrorMessage(message, errorCode) << std::endl;
				}
#else
				static_cast<void>(message);
				static_cast<void>(errorCode);
#endif
				return result;
			}

			static bool WarnAssertTrue(BOOL result, std::string_view message, DWORD errorCode) noexcept
			{
				return WarnAssertTrue(result != FALSE, message, errorCode);
			}

			static bool AssertSuccess(DWORD result, std::string_view message)
			{
				return AssertTrue(result == ERROR_SUCCESS, message, result);
			}

			static bool WarnAssertSuccess(DWORD result, std::string_view message) noexcept
			{
				return WarnAssertTrue(result == ERROR_SUCCESS, message, result);
			}

			static bool AssertSuccess(LSTATUS result, std::string_view message)
			{
				return AssertTrue(result == ERROR_SUCCESS, message, static_cast<DWORD>(result));
			}

			static bool WarnAssertSuccess(LSTATUS result, std::string_view message) noexcept
			{
				return WarnAssertTrue(result == ERROR_SUCCESS, message, static_cast<DWORD>(result));
			}
		};
	}

	template <typename T>
	bool AssertTrue(T result, std::string_view message)
	{
		return Detail::Checker::AssertTrue(result, message, ::GetLastError());
	}

	template <typename T>
	bool WarnAssertTrue(T result, std::string_view message)
	{
		return Detail::Checker::WarnAssertTrue(result, message, ::GetLastError());
	}

	template <typename T>
	bool AssertSuccess(T errorCode, std::string_view message)
	{
		return Detail::Checker::AssertSuccess(errorCode, message);
	}

	template <typename T>
	bool WarnAssertSuccess(T errorCode, std::string_view message)
	{
		return Detail::Checker::WarnAssertSuccess(errorCode, message);
	}
}