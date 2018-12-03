#pragma once

#include "GLib/Win/ErrorCheck.h"
#include "GLib/Win/Handle.h"
#include "GLib/stackorheap.h"

#include <vector>
#include <map>

namespace GLib
{
	namespace Win
	{
		// try again to use fs:path as return values
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

			inline std::string PathOfFileHandle(HANDLE fileHandle, DWORD flags)
			{
				GLib::Util::StackOrHeap<wchar_t, 256> s;
				DWORD pathLen = ::GetFinalPathNameByHandleW(fileHandle, nullptr, 0, flags);
				Util::AssertTrue(pathLen != 0 && pathLen != s.size(), "GetFinalPathNameByHandleW failed");
				s.EnsureSize(static_cast<size_t>(pathLen) + 1);
				pathLen = ::GetFinalPathNameByHandleW(fileHandle, s.Get(), static_cast<DWORD>(s.size()), flags);
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

			inline std::string PathOfModule(HMODULE module)
			{
				GLib::Util::StackOrHeap<wchar_t, 256> s;

				unsigned int length;
				for (;;)
				{
					// this could return prefix "\\?\"
					unsigned int len = ::GetModuleFileNameW(module, s.Get(), static_cast<unsigned int>(s.size()));
					Util::AssertTrue(len != 0, "GetModuleFileName failed");
					if (len < s.size())
					{
						length = len + 1;
						break;
					}
					if (s.size() >= 32768U)
					{
						throw std::runtime_error("Path too long");
					}
					s.EnsureSize(s.size() * 2);
				}

				s.EnsureSize(length);
				length = ::GetModuleFileNameW(module, s.Get(), static_cast<unsigned int>(s.size()));
				Util::AssertTrue(length != 0 && length != s.size(), "GetModuleFileName failed");

				return Cvt::w2a(s.Get());
			}

			inline std::string PathOfProcessHandle(HANDLE process)
			{
				GLib::Util::StackOrHeap<wchar_t, 256> s;

				DWORD requiredSize;
				for (;;)
				{
					auto size = static_cast<DWORD>(s.size());
					Util::AssertTrue(::QueryFullProcessImageNameW(process, 0, s.Get(), &size), "QueryFullProcessImageNameW");
					if (size < s.size())
					{
						requiredSize = size + 1;
						break;
					}
					if (s.size() >= 32768U)
					{
						throw std::runtime_error("Path too long");
					}
					s.EnsureSize(s.size() * 2);
				}

				s.EnsureSize(requiredSize);
				Util::AssertTrue(::QueryFullProcessImageNameW(process, 0, s.Get(), &requiredSize), "QueryFullProcessImageNameW");

				return Cvt::w2a(s.Get());
			}

			inline Handle CreateAutoDeleteFile(const std::string & name)
			{
				HANDLE h = ::CreateFileW(Cvt::a2w(name).c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
					FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
				Util::AssertTrue(!!h, "CreateFile failed");
				return Handle(h);
			}
		}
	}
}