#pragma once

#include <Windows.h>

#include <GLib/Cvt.h>

#include <ostream>
#include <string_view>

namespace GLib::Win::Util
{
	namespace Detail
	{
		template <typename T1, typename T2>
		T1 WindowsCast(T2 t2)
		{
			return reinterpret_cast<T1>(t2); // NOLINT required by legacy windows runtime
		}

		enum class Lang : WORD
		{
			Neutral = LANG_NEUTRAL,				// NOLINT bad macros
			SubDefault = SUBLANG_DEFAULT, // NOLINT
		};

		enum class Flags : ULONG
		{
			AllocateBuffer = FORMAT_MESSAGE_ALLOCATE_BUFFER, // NOLINT
			FromSystem = FORMAT_MESSAGE_FROM_SYSTEM,				 // NOLINT
			IgnoreInserts = FORMAT_MESSAGE_IGNORE_INSERTS,	 // NOLINT
			FromHandle = FORMAT_MESSAGE_FROM_HMODULE,				 // NOLINT
		};

		inline ULONG Make(Lang l1, Lang l2)
		{
			return MAKELANGID(static_cast<WORD>(l1), static_cast<WORD>(l2)); // NOLINT
		}

		inline Flags operator|(Flags a, Flags b)
		{
			return static_cast<Flags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
		}

		inline Flags operator|=(Flags a, Flags b)
		{
			return static_cast<Flags>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
		}
	}

	inline void FormatErrorMessage(std::ostream & stm, ULONG error, const wchar_t * moduleName = nullptr)
	{
		wchar_t * pszMsg = nullptr;
		auto flags = Detail::Flags::AllocateBuffer | Detail::Flags::FromSystem | Detail::Flags::IgnoreInserts;
		if (moduleName != nullptr)
		{
			flags |= Detail::Flags::FromHandle;
		}

		HMODULE module(moduleName != nullptr ? LoadLibraryW(moduleName) : nullptr);
		auto * requiredCast = Detail::WindowsCast<LPWSTR>(&pszMsg);
		auto result =
			FormatMessageW(static_cast<ULONG>(flags), module, error, Make(Detail::Lang::Neutral, Detail::Lang::SubDefault), requiredCast, 0, nullptr);
		if (module != nullptr)
		{
			FreeLibrary(module);
		}

		if (result != 0)
		{
			size_t len = wcslen(pszMsg);
			std::wstring_view wMsg {pszMsg, len};
			constexpr std::wstring_view ending = L"\r\n";
			if (std::equal(ending.rbegin(), ending.rend(), wMsg.rbegin()))
			{
				wMsg = wMsg.substr(0, len - ending.size());
			}

			stm << Cvt::W2A(wMsg);
			::LocalFree(pszMsg);
		}
		else
		{
			stm << "Unknown error";
		}
		if (module != nullptr)
		{
			FreeLibrary(module);
		}
	}
}