#pragma once

#include <GLib/Win/ErrorCheck.h>

#include <algorithm>

namespace GLib::Win
{
	template <typename T>
	std::tuple<const T *, size_t> LoadResource(HINSTANCE instance, unsigned int id, LPCWSTR resourceType)
	{
		HRSRC resource = ::FindResourceW(instance, MAKEINTRESOURCEW(id), resourceType);
		Util::AssertTrue(resource != nullptr, "FindResourceW");

		HGLOBAL resourceData = ::LoadResource(instance, resource);
		Util::AssertTrue(resourceData != nullptr, "LoadResource");

		return {static_cast<const T *>(::LockResource(resourceData)), ::SizeofResource(instance, resource)};
	}

	inline std::string LoadResourceString(HINSTANCE instance, unsigned int id, LPCWSTR resourceType)
	{
		auto [p, size] = LoadResource<char>(instance, id, resourceType);
		std::string s {p, size};
		s.erase(remove(s.begin(), s.end(), '\r'), s.end());
		return s;
	}
}