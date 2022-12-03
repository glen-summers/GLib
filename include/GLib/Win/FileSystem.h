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
		GLib::Util::WideCharBuffer s;

		ULONG const sizeWithFinalTerminator = GetLogicalDriveStringsW(0, nullptr);
		s.EnsureSize(sizeWithFinalTerminator - 1);
		auto const size = GetLogicalDriveStringsW(static_cast<ULONG>(s.Size()), s.Get());
		Util::AssertTrue(size != 0 && size < sizeWithFinalTerminator, "GetLogicalDriveStringsW");

		std::wstring_view const pp {s.Get(), size};
		constexpr wchar_t nul = u'\0';
		for (size_t pos = 0, next = pp.find(nul, pos); next != std::wstring_view::npos; pos = next + 1, next = pp.find(nul, pos))
		{
			drives.push_back(Cvt::W2A(pp.substr(pos, next - 1 - pos)));
		}

		return drives;
	}

	// return an iterator?
	inline std::map<std::string, std::string> DriveMap()
	{
		std::map<std::string, std::string> result;
		GLib::Util::WideCharBuffer s;

		for (auto const & logicalDrive : LogicalDrives())
		{
			ULONG const length = QueryDosDeviceW(Cvt::A2W(logicalDrive).c_str(), s.Get(), static_cast<ULONG>(s.Size()));
			Util::AssertTrue(length != 0, "QueryDosDeviceW");

			s.EnsureSize(length);
			std::size_t const sizeWithoutTwoTrailingNulls = length - 2;
			std::string const dosDeviceName = Cvt::W2A({s.Get(), sizeWithoutTwoTrailingNulls});
			result.emplace(logicalDrive, dosDeviceName);
		}
		return result;
	}

	inline std::string PathOfFileHandle(HandleBase * const fileHandle, ULONG const flags)
	{
		GLib::Util::WideCharBuffer s;
		ULONG length = GetFinalPathNameByHandleW(fileHandle, nullptr, 0, flags);
		Util::AssertTrue(length != 0, "GetFinalPathNameByHandleW");
		s.EnsureSize(length);
		length = GetFinalPathNameByHandleW(fileHandle, s.Get(), static_cast<ULONG>(s.Size()), flags);
		Util::AssertTrue(length != 0 && length < s.Size(), "GetFinalPathNameByHandleW");
		return Cvt::W2A(std::wstring_view {s.Get(), length});
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
		GLib::Util::WideCharBuffer s;

		unsigned int length {};
		for (;;)
		{
			// this could return prefix "\\?\"
			ULONG const len = GetModuleFileNameW(module, s.Get(), static_cast<unsigned int>(s.Size()));
			Util::AssertTrue(len != 0, "GetModuleFileNameW");
			if (len < s.Size())
			{
				length = len + 1;
				break;
			}
			if (s.Size() >= Detail::MaximumPathLength)
			{
				throw std::runtime_error("Path too long");
			}
			s.EnsureSize(s.Size() * 2);
		}

		s.EnsureSize(length);
		length = GetModuleFileNameW(module, s.Get(), static_cast<unsigned int>(s.Size()));
		Util::AssertTrue(length != 0 && length < s.Size(), "GetModuleFileNameW");

		return Cvt::W2A(std::wstring_view {s.Get(), length});
	}

	inline std::string PathOfProcessHandle(HandleBase * const process)
	{
		GLib::Util::WideCharBuffer s;

		ULONG requiredSize {};
		for (;;)
		{
			auto size = static_cast<ULONG>(s.Size());
			Util::AssertTrue(QueryFullProcessImageNameW(process, 0, s.Get(), &size), "QueryFullProcessImageNameW");
			if (size < s.Size())
			{
				requiredSize = size + 1;
				break;
			}
			if (s.Size() >= Detail::MaximumPathLength)
			{
				throw std::runtime_error("Path too long");
			}
			s.EnsureSize(s.Size() * 2);
		}

		s.EnsureSize(requiredSize);
		Util::AssertTrue(QueryFullProcessImageNameW(process, 0, s.Get(), &requiredSize), "QueryFullProcessImageNameW");

		return Cvt::W2A(std::wstring_view {s.Get(), requiredSize});
	}

	inline Handle CreateAutoDeleteFile(std::string const & name)
	{
		HandleBase * const h = CreateFileW(Cvt::A2W(name).c_str(), Detail::Access, 0, nullptr, Detail::Create, Detail::Flags, nullptr);
		Util::AssertTrue(h != nullptr, "CreateFileW");
		return Handle(h);
	}

	inline std::string LongPath(std::string const & name)
	{
		auto const ws = Cvt::A2W(name);
		size_t lenNoTerminator = GetLongPathNameW(ws.c_str(), nullptr, 0);
		Util::AssertTrue(lenNoTerminator != 0, "GetLongPathNameW");

		GLib::Util::WideCharBuffer s;
		s.EnsureSize(lenNoTerminator + 1);

		lenNoTerminator = GetLongPathNameW(ws.c_str(), s.Get(), static_cast<ULONG>(s.Size()));
		Util::AssertTrue(lenNoTerminator != 0, "GetLongPathNameW");
		return Cvt::W2A(std::wstring_view {s.Get(), lenNoTerminator});
	}
}