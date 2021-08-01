#pragma once

#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/Handle.h>

#include <GLib/IcuUtils.h>
#include <GLib/stackorheap.h>

#include <map>
#include <vector>

namespace GLib::Win::FileSystem
{
	// try again to use fs:path as return values?
	namespace Detail
	{
		constexpr auto MaximumPathLength = 32768U;

		enum FileFlags : DWORD
		{
			access = GENERIC_READ | GENERIC_WRITE,												// NOLINT baad macros
			create = CREATE_ALWAYS,																				// NOLINT
			flags = FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, // NOLINT
		};
	}

	// return an iterator?
	inline std::vector<std::string> LogicalDrives()
	{
		std::vector<std::string> drives;
		GLib::Util::WideCharBuffer s;

		DWORD sizeWithFinalTerminator = ::GetLogicalDriveStringsW(0, nullptr);
		s.EnsureSize(static_cast<size_t>(sizeWithFinalTerminator - 1));
		auto size = ::GetLogicalDriveStringsW(static_cast<DWORD>(s.size()), s.Get());
		Util::AssertTrue(size != 0 && size < sizeWithFinalTerminator, "GetLogicalDriveStringsW");

		std::wstring_view pp {s.Get(), size};
		const wchar_t nul = u'\0';
		for (size_t pos = 0, next = pp.find(nul, pos); next != std::wstring_view::npos; pos = next + 1, next = pp.find(nul, pos))
		{
			drives.push_back(Cvt::w2a(pp.substr(pos, next - 1 - pos)));
		}

		return drives;
	}

	// return an iterator?
	inline std::map<std::string, std::string> DriveMap()
	{
		std::map<std::string, std::string> result;
		GLib::Util::WideCharBuffer s;

		for (const auto & logicalDrive : LogicalDrives())
		{
			DWORD length = ::QueryDosDeviceW(Cvt::a2w(logicalDrive).c_str(), s.Get(), static_cast<DWORD>(s.size()));
			Util::AssertTrue(length != 0, "QueryDosDeviceW");

			s.EnsureSize(length);
			size_t sizeWithoutTwoTrailingNulls = length - 2;
			std::string dosDeviceName = Cvt::w2a({s.Get(), sizeWithoutTwoTrailingNulls});
			result.emplace(logicalDrive, dosDeviceName);
		}
		return result;
	}

	inline std::string PathOfFileHandle(HANDLE fileHandle, DWORD flags)
	{
		GLib::Util::WideCharBuffer s;
		DWORD length = ::GetFinalPathNameByHandleW(fileHandle, nullptr, 0, flags);
		Util::AssertTrue(length != 0, "GetFinalPathNameByHandleW");
		s.EnsureSize(length);
		length = ::GetFinalPathNameByHandleW(fileHandle, s.Get(), static_cast<DWORD>(s.size()), flags);
		Util::AssertTrue(length != 0 && length < s.size(), "GetFinalPathNameByHandleW");
		return Cvt::w2a(std::wstring_view {s.Get(), length});
	}

	inline std::string NormalisePath(const std::string & path, const std::map<std::string, std::string> & driveMap)
	{
		// todo: network drives
		for (const auto & [deviceName, logicalName] : driveMap)
		{
			size_t compareSize = logicalName.size();
			if (IcuUtils::CompareNoCase(path, logicalName, compareSize) == IcuUtils::CompareResult::Equal)
			{
				return deviceName + path.substr(compareSize);
			}
		}
		throw std::runtime_error("Drive not mapped"); // or just return path?
	}

	inline std::string PathOfModule(HMODULE module)
	{
		GLib::Util::WideCharBuffer s;

		unsigned int length = 0;
		for (;;)
		{
			// this could return prefix "\\?\"
			unsigned int len = ::GetModuleFileNameW(module, s.Get(), static_cast<unsigned int>(s.size()));
			Util::AssertTrue(len != 0, "GetModuleFileNameW");
			if (len < s.size())
			{
				length = len + 1;
				break;
			}
			if (s.size() >= Detail::MaximumPathLength)
			{
				throw std::runtime_error("Path too long");
			}
			s.EnsureSize(s.size() * 2);
		}

		s.EnsureSize(length);
		length = ::GetModuleFileNameW(module, s.Get(), static_cast<unsigned int>(s.size()));
		Util::AssertTrue(length != 0 && length < s.size(), "GetModuleFileNameW");

		return Cvt::w2a(std::wstring_view {s.Get(), length});
	}

	inline std::string PathOfProcessHandle(HANDLE process)
	{
		GLib::Util::WideCharBuffer s;

		DWORD requiredSize = 0;
		for (;;)
		{
			auto size = static_cast<DWORD>(s.size());
			Util::AssertTrue(::QueryFullProcessImageNameW(process, 0, s.Get(), &size), "QueryFullProcessImageNameW");
			if (size < s.size())
			{
				requiredSize = size + 1;
				break;
			}
			if (s.size() >= Detail::MaximumPathLength)
			{
				throw std::runtime_error("Path too long");
			}
			s.EnsureSize(s.size() * 2);
		}

		s.EnsureSize(requiredSize);
		Util::AssertTrue(::QueryFullProcessImageNameW(process, 0, s.Get(), &requiredSize), "QueryFullProcessImageNameW");

		return Cvt::w2a(std::wstring_view {s.Get(), requiredSize});
	}

	inline Handle CreateAutoDeleteFile(const std::string & name)
	{
		HANDLE h = ::CreateFileW(Cvt::a2w(name).c_str(), Detail::access, 0, nullptr, Detail::create, Detail::flags, nullptr);
		Util::AssertTrue(h != nullptr, "CreateFileW");
		return Handle(h);
	}

	inline std::string LongPath(const std::string & name)
	{
		auto ws = GLib::Cvt::a2w(name);
		size_t lenNoTerminator = ::GetLongPathNameW(ws.c_str(), nullptr, 0);
		Util::AssertTrue(lenNoTerminator != 0, "GetLongPathNameW");

		GLib::Util::WideCharBuffer s;
		s.EnsureSize(lenNoTerminator + 1);

		lenNoTerminator = ::GetLongPathNameW(ws.c_str(), s.Get(), static_cast<DWORD>(s.size()));
		Util::AssertTrue(lenNoTerminator != 0, "GetLongPathNameW");
		return Cvt::w2a(std::wstring_view {s.Get(), lenNoTerminator});
	}
}