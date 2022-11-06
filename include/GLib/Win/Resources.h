#pragma once

#include <GLib/Win/ErrorCheck.h>

namespace GLib::Win
{
	template <typename T>
	std::tuple<const T *, size_t> LoadResource(HINSTANCE instance, unsigned int id, LPCWSTR resourceType)
	{
		HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(id), resourceType); // NOLINT bad macro
		Util::AssertTrue(resource != nullptr, "FindResourceW");

		HGLOBAL resourceData = LoadResource(instance, resource);
		Util::AssertTrue(resourceData != nullptr, "LoadResource");

		return {static_cast<const T *>(LockResource(resourceData)), SizeofResource(instance, resource)};
	}

	inline std::string LoadResourceString(HINSTANCE instance, unsigned int id, LPCWSTR resourceType)
	{
		return std::make_from_tuple<std::string>(LoadResource<char>(instance, id, resourceType));
	}
};
