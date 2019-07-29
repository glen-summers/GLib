#pragma once

#include "GLib/Win/WinException.h"

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include "GLib/Win/DebugStream.h"
#endif

namespace GLib
{
	namespace Win
	{
		namespace Util
		{
			namespace Detail
			{
				inline std::string FormatErrorMessage(const char * message, DWORD error, const wchar_t * moduleName = nullptr)
				{
					std::ostringstream stm;
					stm << message << " : ";
					Util::FormatErrorMessage(stm, error, moduleName);
					if (error >= 0x80000000)
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

				// only allow specific params to prevent accidental int -> bool and misinterpreting win32 results
				// https://stackoverflow.com/questions/175689/can-you-use-keyword-explicit-to-prevent-automatic-conversion-of-method-parameter
				class Checker
				{
				public:
					template<typename T> static void AssertTrue(T value, const char * message)
					{
						UNREFERENCED_PARAMETER(message);
						static_assert(false, "Invalid check parameter, only bool and BOOL allowed");
					}

					static void AssertTrue(bool result, const char * message)
					{
						if (!result)
						{
							Throw(message, ::GetLastError());
						}
					}

					static void AssertTrue(BOOL result, const char * message)
					{
						AssertTrue(result != FALSE, message);
					}

					static void WarnAssertTrue(bool result, const char * message) noexcept
					{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
						if (!result)
						{
							DWORD dwErr = ::GetLastError();
							Debug::Stream() << "GLib warning: " << Detail::FormatErrorMessage(message, dwErr) << std::endl;
						}
#else
						UNREFERENCED_PARAMETER(result);
						UNREFERENCED_PARAMETER(message);
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
				Detail::Checker::AssertTrue(result, message);
			}

			template <typename T> void WarnAssertTrue(T result, const char * message)
			{
				Detail::Checker::WarnAssertTrue(result, message);
			}
		}
	}
}