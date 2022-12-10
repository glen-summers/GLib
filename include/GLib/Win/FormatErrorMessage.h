#pragma once

#include <Windows.h>

#include <GLib/Cvt.h>

#include <ostream>

namespace GLib::Win::Util
{
	namespace Detail
	{
		template <typename T1, typename T2>
		T1 WindowsCast(T2 value)
		{
			return reinterpret_cast<T1>(value); // NOLINT required by legacy windows runtime
		}

		enum class Lang : WORD
		{
			Neutral = LANG_NEUTRAL,				// NOLINT bad macros
			SubDefault = SUBLANG_DEFAULT, // NO-LINT
		};

		enum class Flags : ULONG
		{
			AllocateBuffer = FORMAT_MESSAGE_ALLOCATE_BUFFER, // NOLINT bad macros
			FromSystem = FORMAT_MESSAGE_FROM_SYSTEM,				 // NOLINT
			IgnoreInserts = FORMAT_MESSAGE_IGNORE_INSERTS,	 // NOLINT
			FromHandle = FORMAT_MESSAGE_FROM_HMODULE,				 // NOLINT
		};

		inline ULONG Make(Lang lang1, Lang lang2)
		{
			return MAKELANGID(static_cast<WORD>(lang1), static_cast<WORD>(lang2)); // NOLINT(hicpp-signed-bitwise) bad macro
		}

		inline Flags operator|(Flags flag1, Flags flag2)
		{
			return static_cast<Flags>(static_cast<unsigned int>(flag1) | static_cast<unsigned int>(flag2));
		}

		inline Flags operator|=(Flags flag1, Flags flag2)
		{
			return static_cast<Flags>(static_cast<unsigned int>(flag1) | static_cast<unsigned int>(flag2));
		}
	}

	inline void FormatErrorMessage(std::ostream & stm, ULONG const error, wchar_t const * const moduleName = nullptr)
	{
		wchar_t * pszMsg = nullptr;
		auto flags = Detail::Flags::AllocateBuffer | Detail::Flags::FromSystem | Detail::Flags::IgnoreInserts;
		if (moduleName != nullptr)
		{
			flags |= Detail::Flags::FromHandle;
		}

		HMODULE const module(moduleName != nullptr ? LoadLibraryW(moduleName) : nullptr);
		auto * requiredCast = Detail::WindowsCast<LPWSTR>(&pszMsg);
		auto const result =
			FormatMessageW(static_cast<ULONG>(flags), module, error, Make(Detail::Lang::Neutral, Detail::Lang::SubDefault), requiredCast, 0, nullptr);
		if (module != nullptr)
		{
			FreeLibrary(module);
		}

		if (result != 0)
		{
			size_t const len = wcslen(pszMsg);
			std::wstring_view wMsg {pszMsg, len};
			constexpr std::wstring_view ending = L"\r\n";
			if (std::equal(ending.rbegin(), ending.rend(), wMsg.rbegin()))
			{
				wMsg = wMsg.substr(0, len - ending.size());
			}

			stm << Cvt::W2A(wMsg);
			LocalFree(pszMsg);
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