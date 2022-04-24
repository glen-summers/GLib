#pragma once

#include <GLib/Win/FormatErrorMessage.h>
#include <GLib/Win/WinException.h>

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include <GLib/Win/DebugStream.h>
#endif

#include <sstream>

EXTERN_C IMAGE_DOS_HEADER __ImageBase; // NOLINT required

namespace GLib::Win::Util
{
	namespace Exp
	{
		template <typename T>
		concept ULong = std::same_as<T, ULONG>;

		template <typename T>
		concept Status = std::same_as<T, LSTATUS>;

		template <typename T>
		concept BOOL = std::same_as<T, ::BOOL>;

		template <typename T>
		concept Bool = std::same_as<T, bool>;
	}

	namespace Detail
	{
		inline bool IsError(Exp::ULong auto value)
		{
			return IS_ERROR(value); // NOLINT bad macro
		}

		inline std::string FormatErrorMessage(std::string_view message, Exp::ULong auto error, const wchar_t * moduleName = {})
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

		__declspec(noreturn) inline void Throw(std::string_view message, Exp::ULong auto result)
		{
			std::string formattedMessage = FormatErrorMessage(message, result);

#ifdef _DEBUG // || defined(GLIB_DEBUG)
			Debug::Stream() << "WinException : " << formattedMessage << std::endl;
#endif
			throw WinException(formattedMessage, result);
		}

		inline bool AssertTrue(Exp::Bool auto result, std::string_view message, Exp::ULong auto errorCode)
		{
			if (!result)
			{
				Throw(message, errorCode);
			}
			return result;
		}

		inline bool AssertTrue(Exp::BOOL auto result, std::string_view message, Exp::ULong auto errorCode)
		{
			return AssertTrue(result != FALSE, message, errorCode);
		}

		inline bool WarnAssertTrue(Exp::Bool auto result, std::string_view message, Exp::ULong auto errorCode) noexcept
		{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
			if (!result)
			{
				Debug::Stream() << "GLib warning: " << FormatErrorMessage(message, errorCode) << std::endl;
			}
#else
			static_cast<void>(message);
			static_cast<void>(errorCode);
#endif
			return result;
		}

		inline bool WarnAssertTrue(Exp::BOOL auto result, std::string_view message, Exp::ULong auto errorCode) noexcept
		{
			return WarnAssertTrue(result != FALSE, message, errorCode);
		}

		inline bool AssertSuccess(Exp::ULong auto result, std::string_view message)
		{
			return AssertTrue(result == ERROR_SUCCESS, message, result);
		}

		inline bool WarnAssertSuccess(Exp::ULong auto result, std::string_view message) noexcept
		{
			return WarnAssertTrue(result == ERROR_SUCCESS, message, result);
		}

		inline bool AssertSuccess(Exp::Status auto result, std::string_view message)
		{
			return AssertTrue(result == ERROR_SUCCESS, message, static_cast<ULONG>(result));
		}

		inline bool WarnAssertSuccess(Exp::Status auto result, std::string_view message) noexcept
		{
			return WarnAssertTrue(result == ERROR_SUCCESS, message, static_cast<ULONG>(result));
		}
	}

	inline bool AssertTrue(Exp::Bool auto result, std::string_view message)
	{
		return Detail::AssertTrue(result, message, GetLastError());
	}

	inline bool AssertTrue(Exp::BOOL auto result, std::string_view message)
	{
		return Detail::AssertTrue(result, message, GetLastError());
	}

	inline bool WarnAssertTrue(Exp::Bool auto result, std::string_view message)
	{
		return Detail::WarnAssertTrue(result, message, GetLastError());
	}

	inline bool WarnAssertTrue(Exp::BOOL auto result, std::string_view message)
	{
		return Detail::WarnAssertTrue(result, message, GetLastError());
	}

	inline bool AssertSuccess(Exp::Status auto errorCode, std::string_view message)
	{
		return Detail::AssertSuccess(errorCode, message);
	}

	inline bool WarnAssertSuccess(Exp::Status auto errorCode, std::string_view message)
	{
		return Detail::WarnAssertSuccess(errorCode, message);
	}
}