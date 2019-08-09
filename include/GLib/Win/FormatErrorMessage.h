#pragma once

#include <Windows.h>

#include "GLib/cvt.h"

#include <ostream>
#include <string_view>

namespace GLib::Win::Util
{
	namespace Detail
	{
		inline DWORD FormatMessageCast(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, LPWSTR* lpBuffer)
		{
			return ::FormatMessageW(dwFlags, lpSource, dwMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // NOLINT(hicpp-signed-bitwise) baad macro
				reinterpret_cast<LPWSTR>(lpBuffer), 0, nullptr); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
		}
	}

	inline void FormatErrorMessage(std::ostream & stm, unsigned int error, const wchar_t * moduleName = nullptr)
	{
		wchar_t *pszMsg;
		int flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS; // NOLINT(hicpp-signed-bitwise) baad macro
		if (moduleName != nullptr)
		{
			flags |= FORMAT_MESSAGE_FROM_HMODULE; // NOLINT(hicpp-signed-bitwise) baad macro
		}

		HMODULE module(moduleName != nullptr ? ::LoadLibraryW(moduleName) : nullptr);
		if (Detail::FormatMessageCast(flags, module, error, &pszMsg) != 0)
		{
			size_t len = ::wcslen(pszMsg);
			std::wstring_view wMsg { pszMsg,  len }; 
			constexpr std::wstring_view ending = L"\r\n";
			if (std::equal(ending.rbegin(), ending.rend(), wMsg.rbegin()))
			{
				wMsg = wMsg.substr(0, len - ending.size());
			}

			stm << Cvt::w2a(wMsg);
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