#pragma once

#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/Handle.h>

#include <GLib/IcuUtils.h>
#include <GLib/StackOrHeap.h>

#include <map>
#include <vector>

namespace GLib::Win::FileSystem
{
	// try again to use fs:path as return values?
	namespace Detail
	{
		constexpr auto MaximumPathLength = 32768U;

		enum FileFlags : ULONG
		{
			Access = GENERIC_READ | GENERIC_WRITE,												// NOLINT bad macros
			Create = CREATE_ALWAYS,																				// NOLINT
			Flags = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, // NOLINT
		};
	}

	// return an iterator?
	inline std::vector<std::string> LogicalDrives()
	{
		std::vector<std::string> drives;
		GLib::Util::WideCharBuffer buffer;

		ULONG const sizeWithFinalTerminator = GetLogicalDriveStringsW(0, nullptr);
		buffer.EnsureSize(sizeWithFinalTerminator - 1);
		auto const size = GetLogicalDriveStringsW(static_cast<ULONG>(buffer.Size()), buffer.Get());
		Util::AssertTrue(size != 0 && size < sizeWithFinalTerminator, "GetLogicalDriveStringsW");

		std::wstring_view const value {buffer.Get(), size};
		constexpr wchar_t nul = u'\0';
		for (size_t pos = 0, next = value.find(nul, pos); next != std::wstring_view::npos; pos = next + 1, next = value.find(nul, pos))
		{
			drives.push_back(Cvt::W2A(value.substr(pos, next - 1 - pos)));
		}

		return drives;
	}

	// return an iterator?
	inline std::map<std::string, std::string> DriveMap()
	{
		std::map<std::string, std::string> result;
		GLib::Util::WideCharBuffer buffer;

		for (auto const & logicalDrive : LogicalDrives())
		{
			ULONG const length = QueryDosDeviceW(Cvt::A2W(logicalDrive).c_str(), buffer.Get(), static_cast<ULONG>(buffer.Size()));
			Util::AssertTrue(length != 0, "QueryDosDeviceW");

			buffer.EnsureSize(length);
			std::size_t const sizeWithoutTwoTrailingNulls = length - 2;
			std::string const dosDeviceName = Cvt::W2A({buffer.Get(), sizeWithoutTwoTrailingNulls});
			result.emplace(logicalDrive, dosDeviceName);
		}
		return result;
	}

	inline std::string PathOfFileHandle(HandleBase * const fileHandle, ULONG const flags)
	{
		GLib::Util::WideCharBuffer buffer;
		ULONG length = GetFinalPathNameByHandleW(fileHandle, nullptr, 0, flags);
		Util::AssertTrue(length != 0, "GetFinalPathNameByHandleW");
		buffer.EnsureSize(length);
		length = GetFinalPathNameByHandleW(fileHandle, buffer.Get(), static_cast<ULONG>(buffer.Size()), flags);
		Util::AssertTrue(length != 0 && length < buffer.Size(), "GetFinalPathNameByHandleW");
		return Cvt::W2A(std::wstring_view {buffer.Get(), length});
	}

	inline std::string NormalisePath(std::string const & path, std::map<std::string, std::string> const & driveMap)
	{
		// todo: network drives
		for (auto const & [deviceName, logicalName] : driveMap)
		{
			size_t const compareSize = logicalName.size();
			if (IcuUtils::CompareNoCase(path, logicalName, compareSize) == IcuUtils::CompareResult::Equal)
			{
				return deviceName + path.substr(compareSize);
			}
		}
		throw std::runtime_error("Drive not mapped"); // or just return path?
	}

	inline std::string PathOfModule(HMODULE const module)
	{
		GLib::Util::WideCharBuffer buffer;

		unsigned int length {};
		for (;;)
		{
			// this could return prefix "\\?\"
			ULONG const len = GetModuleFileNameW(module, buffer.Get(), static_cast<unsigned int>(buffer.Size()));
			Util::AssertTrue(len != 0, "GetModuleFileNameW");
			if (len < buffer.Size())
			{
				length = len + 1;
				break;
			}
			if (buffer.Size() >= Detail::MaximumPathLength)
			{
				throw std::runtime_error("Path too long");
			}
			buffer.EnsureSize(buffer.Size() * 2);
		}

		buffer.EnsureSize(length);
		length = GetModuleFileNameW(module, buffer.Get(), static_cast<unsigned int>(buffer.Size()));
		Util::AssertTrue(length != 0 && length < buffer.Size(), "GetModuleFileNameW");

		return Cvt::W2A(std::wstring_view {buffer.Get(), length});
	}

	inline std::string PathOfProcessHandle(HandleBase * const process)
	{
		GLib::Util::WideCharBuffer buffer;

		ULONG requiredSize {};
		for (;;)
		{
			auto size = static_cast<ULONG>(buffer.Size());
			Util::AssertTrue(QueryFullProcessImageNameW(process, 0, buffer.Get(), &size), "QueryFullProcessImageNameW");
			if (size < buffer.Size())
			{
				requiredSize = size + 1;
				break;
			}
			if (buffer.Size() >= Detail::MaximumPathLength)
			{
				throw std::runtime_error("Path too long");
			}
			buffer.EnsureSize(buffer.Size() * 2);
		}

		buffer.EnsureSize(requiredSize);
		Util::AssertTrue(QueryFullProcessImageNameW(process, 0, buffer.Get(), &requiredSize), "QueryFullProcessImageNameW");

		return Cvt::W2A(std::wstring_view {buffer.Get(), requiredSize});
	}

	inline Handle CreateAutoDeleteFile(std::string const & name)
	{
		HandleBase * const handle = CreateFileW(Cvt::A2W(name).c_str(), Detail::Access, 0, nullptr, Detail::Create, Detail::Flags, nullptr);
		Util::AssertTrue(handle != nullptr, "CreateFileW");
		return Handle(handle);
	}

	inline std::string LongPath(std::string const & name)
	{
		auto const wideValue = Cvt::A2W(name);
		size_t lenNoTerminator = GetLongPathNameW(wideValue.c_str(), nullptr, 0);
		Util::AssertTrue(lenNoTerminator != 0, "GetLongPathNameW");

		GLib::Util::WideCharBuffer buffer;
		buffer.EnsureSize(lenNoTerminator + 1);

		lenNoTerminator = GetLongPathNameW(wideValue.c_str(), buffer.Get(), static_cast<ULONG>(buffer.Size()));
		Util::AssertTrue(lenNoTerminator != 0, "GetLongPathNameW");
		return Cvt::W2A(std::wstring_view {buffer.Get(), lenNoTerminator});
	}
}