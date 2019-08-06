#pragma once

#ifdef __GNUG__
#include <cxxabi.h>
#endif

#ifdef __linux__
#include "GLib/stackorheap.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <experimental/filesystem>
#elif _WIN32
#include <filesystem>
#include "Win/Process.h"
#else
//?
#endif

#include <array>
#include <ctime>
#include <stdexcept>
#include <string>
#include <string_view>

namespace GLib::Compat
{
#ifdef __linux__
	namespace filesystem = std::experimental::filesystem;
#elif _WIN32
	namespace filesystem = std::filesystem;
#else
	//?
#endif

	inline void LocalTime(tm & tm, const time_t & t)
	{
#ifdef __linux__
		localtime_r(&t, &tm);
#elif _MSC_VER
		localtime_s(&tm, &t);
#else
		//?
#endif
	}

	inline void GmTime(tm & tm, const time_t & t)
	{
#ifdef __linux__
		gmtime_r(&t, &tm);
#elif _MSC_VER
		gmtime_s(&tm, &t);
#else
		//?
#endif
	}

	inline void StrError(const char * prefix)
	{
		constexpr auto ErrorBufferSize = 256;
		std::array<char, ErrorBufferSize> err{};
		char * msg;
#ifdef __linux__
		msg = strerror_r(errno, err.data(), err.size());
#elif _MSC_VER
		strerror_s(msg = err.data(), err.size(), errno);
#else
		//?
#endif
		throw std::logic_error(std::string(prefix)+ " : " + msg);
	}


	inline std::string Unmangle(const std::string & name)
	{
#ifdef _MSC_VER
		constexpr std::string_view Class = "class ";
		constexpr std::string_view Struct = "struct ";

		return name.compare(0, Class.size(), Class) == 0
			? name.substr(Class.size())
			: name.compare(0, Struct.size(), Struct) == 0
				? name.substr(Struct.size())
				: name; // etc...
#elif __GNUG__
		int status = -1;
		std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name.c_str(), NULL, NULL, &status), std::free };
		return status == 0 ? res.get() : name;
#else
		return name;
#endif
	}

	inline int64_t ProcessId()
	{
#ifdef _MSC_VER
			return Win::Process::CurrentId();
#elif __linux__
			return ::getpid();
#else
			// getprogname?
			//?
#endif
	}

	inline std::string ProcessPath()
	{
#ifdef _MSC_VER
		return Win::Process::CurrentPath();
#elif __linux__

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
#else
		//?
#endif
	}

	inline std::string ProcessName()
	{
		return filesystem::path(ProcessPath()).filename().u8string();
	}
}