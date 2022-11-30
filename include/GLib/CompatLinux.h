#pragma once

#if defined(__linux__) && defined(__GNUG__)

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>

#include <cxxabi.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <GLib/Cvt.h>

namespace GLib::Compat
{
	inline void AssertTrue(bool value, char const * prefix, int const error)
	{
		if (!value)
		{
			constexpr auto errorBufferSize = 256;
			std::array<char, errorBufferSize> err {};
			char * msg = strerror_r(error, err.data(), err.size());
			throw std::runtime_error(std::string(prefix) + " : " + msg);
		}
	}

	inline void LocalTime(tm & tm, time_t const & t)
	{
		auto result = localtime_r(&t, &tm);
		AssertTrue(result != nullptr, "LocalTime", errno);
	}

	inline void GmTime(tm & tm, time_t const & t)
	{
		auto result = gmtime_r(&t, &tm);
		AssertTrue(result != nullptr, "GmTime", errno);
	}

	inline void SetEnv(char const * name, char const * value)
	{
		int result = setenv(name, value, 1);
		AssertTrue(result != -1, "setenv", errno);
	}

	inline std::optional<std::string> GetEnv(char const * name)
	{
		char const * value = getenv(name);
		return value != nullptr ? std::optional<std::string> {value} : std::nullopt;
	}

	inline void UnsetEnv(char const * name)
	{
		int result = unsetenv(name);
		AssertTrue(result != -1, "UnsetEnv", errno);
	}

	inline std::string Unmangle(std::string const & name)
	{
		int status = -1;
		std::unique_ptr<char, void (*)(void *)> res {abi::__cxa_demangle(name.c_str(), NULL, NULL, &status), std::free};
		return status == 0 ? res.get() : name;
	}

	inline int64_t ProcessId()
	{
		return getpid();
	}

	inline std::string ProcessPath()
	{
		return Cvt::P2A(std::filesystem::read_symlink("/proc/self/exe"));
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
		return Cvt::P2A(std::filesystem::path(ProcessPath()).filename());
	}

	inline void TzSet()
	{
		tzset();
	}
}

#endif