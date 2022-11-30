#pragma once

#include <GLib/Win/ErrorCheck.h>

namespace GLib::Win
{
	template <typename T>
	std::tuple<T const *, size_t> LoadResource(HINSTANCE const instance, unsigned int const id, wchar_t const * const resourceType)
	{
		HRSRC const resource = FindResourceW(instance, MAKEINTRESOURCEW(id), resourceType); // NOLINT bad macro
		Util::AssertTrue(resource != nullptr, "FindResourceW");

		HGLOBAL const resourceData = LoadResource(instance, resource);
		Util::AssertTrue(resourceData != nullptr, "LoadResource");

		return {static_cast<T const *>(LockResource(resourceData)), SizeofResource(instance, resource)};
	}

	inline std::string LoadResourceString(HINSTANCE const instance, unsigned int const id, wchar_t const * const resourceType)
	{
		return std::make_from_tuple<std::string>(LoadResource<char>(instance, id, resourceType));
	}
};
