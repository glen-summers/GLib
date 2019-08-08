#pragma once

#include "GLib/stackorheap.h"

#include <experimental/filesystem>

#include <cxxabi.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace GLib::Compat
{
	namespace filesystem = std::experimental::filesystem;

	inline void LocalTime(tm & tm, const time_t & t)
	{
		localtime_r(&t, &tm);
	}

	inline void GmTime(tm & tm, const time_t & t)
	{
		gmtime_r(&t, &tm);
	}

	inline void StrError(const char * prefix)
	{
		constexpr auto ErrorBufferSize = 256;
		std::array<char, ErrorBufferSize> err{};
		char * msg = strerror_r(errno, err.data(), err.size());
		throw std::runtime_error(std::string(prefix)+ " : " + msg);
	}

	inline std::string Unmangle(const std::string & name)
	{
		int status = -1;
		std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name.c_str(), NULL, NULL, &status), std::free };
		return status == 0 ? res.get() : name;
	}

	inline int64_t ProcessId()
	{
			return ::getpid();
	}

	inline std::string ProcessPath()
	{
		std::ostringstream s;
		s << "/proc/" << ::getpid() << "/exe";

		struct stat sb;
		auto ret = ::lstat(s.str().c_str(), &sb);
		if (ret == -1) // errno?
		{
			StrError("lstat failed"); // test
		}

		Util::StackOrHeap<char, 256> soh;
		soh.EnsureSize(sb.st_size + 1);

		int readBytes = ::readlink(s.str().c_str(), soh.Get(), soh.size());
		if (readBytes < 0)
		{
			StrError("readlink failed"); // test
		}
		soh.Get()[readBytes] = '\0';
		return soh.Get();
	}

	inline std::string ProcessName()
	{
		return filesystem::path(ProcessPath()).filename().u8string();
	}
}