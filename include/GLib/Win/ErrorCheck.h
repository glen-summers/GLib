#pragma once

#include "GLib/Win/WinException.h"
//#include "DebugStream.h"

namespace GLib
{
	namespace Win
	{
		namespace Util
		{
			namespace Detail
			{
				__declspec(noreturn) inline void Throw(const char * message, DWORD result)
				{
					WinException(message, result);
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
						UNREFERENCED_PARAMETER(result);
						UNREFERENCED_PARAMETER(message);
						if (!result)
						{
							//Debug::Write(WinException(message, ::GetLastError()).what()); // avoid exception, avoid Debug?
						}
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
		}
	}
}