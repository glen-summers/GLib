#pragma once

#include "ErrorCheck.h"
#include "GLib/stackorheap.h"

#include <vector>
#include <map>

namespace GLib
{
	namespace Win
	{
		namespace FileSystem
		{
			// return an iterator?
			inline std::vector<std::string> LogicalDrives()
			{
				std::vector<std::string> drives;
				GLib::Util::StackOrHeap<wchar_t, 256> s;

				DWORD sizeWithoutTerminator = ::GetLogicalDriveStringsW(0, nullptr);
				s.EnsureSize(static_cast<size_t>(sizeWithoutTerminator) + 1);
				sizeWithoutTerminator = ::GetLogicalDriveStringsW(static_cast<DWORD>(s.size()), s.Get());
				Util::AssertTrue(sizeWithoutTerminator != 0 && sizeWithoutTerminator != s.size() - 1, "GetLogicalDriveStrings failed");

				const wchar_t * p = s.Get();
				while (*p)
				{
					auto drive = Cvt::w2a(p);
					const auto afterSlash = drive.find_last_not_of('\\');
					drive.erase(afterSlash + 1);
					drives.push_back(drive);
					p += ::wcslen(p) + 1;
				}
				return drives;
			}

			// return an iterator?
			inline std::map<std::string, std::string> DriveMap()
			{
				std::map<std::string, std::string> result;
				GLib::Util::StackOrHeap<wchar_t, 256> s;

				for (const auto & logicalDrive : LogicalDrives())
				{
					// need grow buffer...
					DWORD ret = ::QueryDosDeviceW(Cvt::a2w(logicalDrive).c_str(), s.Get(), static_cast<DWORD>(s.size()));
					Util::AssertTrue(ret != 0, "QueryDosDeviceW");
					std::string dosDeviceName = Cvt::w2a(s.Get());
					result.insert({ logicalDrive, dosDeviceName });
				}
				return result;
			}

			inline std::string PathOfHandle(HANDLE fileHandle)
			{
				GLib::Util::StackOrHeap<wchar_t, 256> s;
				DWORD pathLen = ::GetFinalPathNameByHandleW(fileHandle, nullptr, 0, VOLUME_NAME_NT);
				Util::AssertTrue(pathLen != 0 && pathLen != s.size(), "GetFinalPathNameByHandleW failed");
				s.EnsureSize(static_cast<size_t>(pathLen) + 1);
				pathLen = ::GetFinalPathNameByHandleW(fileHandle, s.Get(), static_cast<DWORD>(s.size()), VOLUME_NAME_NT);
				Util::AssertTrue(pathLen != 0 && pathLen != s.size(), "GetFinalPathNameByHandleW failed");
				return Cvt::w2a(s.Get());
			}

			inline std::string NormalisePath(const std::string & path, const std::map<std::string, std::string> & driveMap)
			{
				// todo: network drives
				for (const auto & drive : driveMap)
				{
					const auto& deviceName = drive.first;
					const auto& logicalName = drive.second;

					size_t compareSize = logicalName.size();
					if (Cvt::CompareNoCase(path.c_str(), compareSize, logicalName.c_str(), compareSize) == Cvt::CompareResult::Equal)
					{
						return deviceName + path.substr(compareSize);
					}
				}
				throw std::runtime_error("Drive not mapped"); // or just return path?
			}
		}
	}
}
