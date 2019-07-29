#pragma once

#include <Windows.h>

#include "GLib/cvt.h"

#include <ostream>

namespace GLib
{
	namespace Win
	{
		namespace Util
		{
			namespace Detail
			{
				inline static constexpr const char * crlf = "\r\n";

				inline DWORD FormatMessageCast(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, LPWSTR* lpBuffer)
				{
					return ::FormatMessageW(dwFlags, lpSource, dwMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						reinterpret_cast<LPWSTR>(lpBuffer), 0, nullptr); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
				}
			}

			inline void FormatErrorMessage(std::ostream & stm, unsigned int error, const wchar_t * moduleName = nullptr)
			{
				wchar_t *pszMsg;
				int flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
				if (moduleName != nullptr)
				{
					flags |= FORMAT_MESSAGE_FROM_HMODULE;
				}

				HMODULE module(moduleName != nullptr ? ::LoadLibraryW(moduleName) : nullptr);
				if (Detail::FormatMessageCast(flags, module, error, &pszMsg) != 0)
				{
					size_t len = ::lstrlenW(pszMsg);
					std::wstring_view wMsg { pszMsg,  len }; 
					const std::wstring_view ending = L"\r\n";
					if (std::equal(ending.rbegin(), ending.rend(), wMsg.rbegin()))
					{
						wMsg = wMsg.substr(0, len - 2);
					}

					stm << Cvt::w2a(std::wstring{wMsg}); // avoid double alloc
					::LocalFree(pszMsg);
				}
				else
				{
					stm << "Unknown error";
				}
				if (module != nullptr)
				{
					::FreeLibrary(module);
				}
			}
		}
	}
}