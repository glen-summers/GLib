#pragma once

#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/Handle.h>

namespace GLib::Win
{
	template <typename T>
	std::tuple<T const *, size_t> LoadResource(InstanceBase * const instance, unsigned int const idValue, wchar_t const * const resourceType)
	{
		HRSRC const resource = FindResourceW(instance, MAKEINTRESOURCEW(idValue), resourceType); // NOLINT bad macro
		Util::AssertTrue(resource != nullptr, "FindResourceW");

#pragma warning(push)
#pragma warning(disable : 6387)
		HGLOBAL const resourceData = LoadResource(instance, resource);
		Util::AssertTrue(resourceData != nullptr, "LoadResource");

		return {static_cast<T const *>(LockResource(resourceData)), SizeofResource(instance, resource)};
#pragma warning(pop)
	}

	inline std::string LoadResourceString(InstanceBase * const instance, unsigned int const idValue, wchar_t const * const resourceType)
	{
		return std::make_from_tuple<std::string>(LoadResource<char>(instance, idValue, resourceType));
	}
}
