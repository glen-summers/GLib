#pragma once

#include "GLib/stackorheap.h"
#include "ErrorCheck.h"

#include <filesystem>

namespace GLib
{
	namespace Win
	{
		namespace Module
		{
			inline std::filesystem::path FileName(HINSTANCE instance)
			{
				GLib::Util::StackOrHeap<wchar_t, 256> s;

				unsigned int length;
				for (;;)
				{
					// this could return prefix "\\?\"
					unsigned int len = ::GetModuleFileNameW(instance, s.Get(), static_cast<unsigned int>(s.size()));
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
				length = ::GetModuleFileNameW(instance, s.Get(), static_cast<unsigned int>(s.size()));
				Util::AssertTrue(length != 0 && length != s.size(), "GetModuleFileName failed");

				return s.Get();
			}
		}
	}
}
