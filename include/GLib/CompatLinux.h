#pragma once

#include <algorithm>
#include <experimental/filesystem>
#include <fstream>
#include <optional>
#include <sstream>

#include <cxxabi.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <GLib/cvt.h>

namespace GLib::Compat
{
	namespace filesystem = std::experimental::filesystem;

	inline void AssertTrue(bool value, const char * prefix, int error)
	{
		if (!value)
		{
			constexpr auto ErrorBufferSize = 256;
			std::array<char, ErrorBufferSize> err {};
			char * msg = ::strerror_r(error, err.data(), err.size());
			throw std::runtime_error(std::string(prefix) + " : " + msg);
		}
	}

	inline void LocalTime(tm & tm, const time_t & t)
	{
		auto result = ::localtime_r(&t, &tm);
		AssertTrue(result != nullptr, "localtime_r", errno);
	}

	inline void GmTime(tm & tm, const time_t & t)
	{
		auto result = ::gmtime_r(&t, &tm);
		AssertTrue(result != nullptr, "gmtime_r", errno);
	}

	inline void SetEnv(const char * name, const char * value)
	{
		int result = ::setenv(name, value, 1);
		AssertTrue(result != -1, "setenv", errno);
	}

	inline std::optional<std::string> GetEnv(const char * name)
	{
		const char * value = ::getenv(name);
		return value != nullptr ? std::optional<std::string> {value} : std::nullopt;
	}

	inline void UnsetEnv(const char * name)
	{
		int result = ::unsetenv(name);
		AssertTrue(result != -1, "unsetenv", errno);
	}

	inline std::string Unmangle(const std::string & name)
	{
		int status = -1;
		std::unique_ptr<char, void (*)(void *)> res {abi::__cxa_demangle(name.c_str(), NULL, NULL, &status), std::free};
		return status == 0 ? res.get() : name;
	}

	inline int64_t ProcessId()
	{
		return ::getpid();
	}

	inline std::string ProcessPath()
	{
		return Cvt::p2a(filesystem::read_symlink("/proc/self/exe"));
	}

	inline std::string CommandLine()
	{
		std::ifstream t("/proc/self/cmdline", std::ios::binary);
		if (!t)
		{
			throw std::runtime_error("Cmdline read failed");
		}
		std::stringstream ss;
		ss << t.rdbuf();
		std::string r {ss.str()};
		std::replace(r.begin(), r.end(), '\0', ' ');
		return r;
	}

	inline std::string ProcessName()
	{
		return Cvt::p2a(filesystem::path(ProcessPath()).filename());
	}

	inline void TzSet()
	{
		::tzset();
	}
}