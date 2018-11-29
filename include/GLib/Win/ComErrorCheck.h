#pragma once

#include "GLib/Win/ComException.h"

#ifdef _DEBUG // || defined(GLIB_DEBUG)
#include "GLib/Win/DebugStream.h"
#endif

namespace GLib
{
	namespace Win
	{
		namespace Detail
		{
			__declspec(noreturn) inline void Throw(const char * message, HRESULT hr)
			{
				ComException e(hr, message);
#ifdef _DEBUG // || defined(GLIB_DEBUG)
				Debug::Stream() << "ComException : " << e.what() << std::endl;
#endif
				throw e;
			}
		}

		inline void CheckHr(HRESULT hr, const char * message)
		{
			if (FAILED(hr))
			{
				Detail::Throw(message, hr);
			}
		}

		inline void WarnHr(HRESULT hr, const char * message) noexcept
		{
#ifdef _DEBUG // || defined(GLIB_DEBUG)
			bool result = SUCCEEDED(hr);
			if (!result)
			{
				Debug::Stream() << "ComException : " << ComException(hr, message).what() << std::endl;
			}
#else
				UNREFERENCED_PARAMETER(hr);
				UNREFERENCED_PARAMETER(message);
#endif
		}
	}
}
