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
			inline void FormatErrorMessage(std::ostream & stm, unsigned int error, const wchar_t * moduleName = nullptr)
			{
				wchar_t *pszMsg;
				int flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
				if (moduleName)
				{
					flags |= FORMAT_MESSAGE_FROM_HMODULE;
				}

				// no smart holders so low dependency and and avoid circular references in headers
				HMODULE module(moduleName ? ::LoadLibraryW(moduleName) : nullptr);
				if (::FormatMessageW(flags, module, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&pszMsg), 0, nullptr) != 0)
				{
					int nLen = lstrlenW(pszMsg);
					if (nLen > 1 && pszMsg[nLen - 1] == '\n')
					{
						pszMsg[nLen - 1] = 0;
						if (pszMsg[nLen - 2] == '\r')
							pszMsg[nLen - 2] = 0;
					}
					stm << Cvt::w2a(pszMsg);
					::LocalFree(pszMsg);
				}
				else
				{
					stm << "Unknown error";
				}
				if (module)
				{
					::FreeLibrary(module);
				}
			}
		}
	}
}