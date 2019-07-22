#pragma once

#include "GLib/Win/ErrorCheck.h"

namespace GLib
{
	namespace Win
	{
		inline std::string LoadResourceString(HINSTANCE instance, unsigned int id)
		{
			LPWSTR p = nullptr;
			int length = ::LoadStringW(instance, id, reinterpret_cast<LPWSTR>(&p), 0);
			Util::AssertTrue(length != 0, "LoadString");
			return Cvt::w2a(std::wstring(p, static_cast<size_t>(length)));
		}

		inline void LoadResourceFile(HMODULE module, unsigned int id, LPCWSTR resourceType, size_t & size, const void *& data)
		{
			HRSRC resource = ::FindResourceW(module, MAKEINTRESOURCEW(id), resourceType);
			Util::AssertTrue(resource != nullptr, "FindResource failed");

			HGLOBAL resourceData = ::LoadResource(module, resource);
			size = ::SizeofResource(module, resource);
			data = static_cast<const void*>(::LockResource(resourceData));
		}
	}
}