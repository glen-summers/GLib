#pragma once

#include "Win/Process.h"

#include <array>
#include <ctime>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

namespace GLib::Compat
{
	namespace filesystem = std::filesystem;

	inline void LocalTime(tm & tm, const time_t & t)
	{
		localtime_s(&tm, &t);
	}

	inline void GmTime(tm & tm, const time_t & t)
	{
		gmtime_s(&tm, &t);
	}

	[[noreturn]] inline void StrError(const char * prefix, errno_t error = errno)
	{
		// wide?
		constexpr auto ErrorBufferSize = 256;
		std::array<char, ErrorBufferSize> err{};
		char * msg;
		strerror_s(msg = err.data(), err.size(), error);
		throw std::runtime_error(std::string(prefix)+ " : " + msg);
	}

	inline void SetEnv(const char * name, const char * value)
	{
		auto wideName = GLib::Cvt::a2w(name);
		auto wideValue = GLib::Cvt::a2w(value);

		errno_t err = ::_wputenv_s(wideName.c_str(), wideValue.c_str());
		if (err != 0)
		{
			StrError("_putenv_s failed", err);
		}
	}

	inline std::optional<std::string> GetEnv(const char * name)
	{
		auto wideName = GLib::Cvt::a2w(name);

		GLib::Util::WideCharBuffer tmp;
		size_t len;
		errno_t err = ::_wgetenv_s(&len, tmp.Get(), tmp.size(), wideName.c_str());
		if (len == 0)
		{
			return {};
		}

		if (len > tmp.size())
		{
			tmp.EnsureSize(len);
			err = ::_wgetenv_s(&len, tmp.Get(), tmp.size(), wideName.c_str());
		}
		if (err != 0)
		{
			StrError("getenv_s failed", err);
		}
		return Cvt::w2a({tmp.Get(), len-1});
	}

	inline void UnsetEnv(const char * name)
	{
		SetEnv(name, "");
	}

	inline std::string Unmangle(const std::string & name)
	{
		constexpr std::string_view Class = "class ";
		constexpr std::string_view Struct = "struct ";

		return name.compare(0, Class.size(), Class) == 0
			? name.substr(Class.size())
			: name.compare(0, Struct.size(), Struct) == 0
				? name.substr(Struct.size())
				: name; // etc...
	}

	inline int64_t ProcessId()
	{
		return Win::Process::CurrentId();
	}

	inline std::string ProcessPath()
	{
		return Win::Process::CurrentPath();
	}

	inline std::string ProcessName()
	{
		return filesystem::path(ProcessPath()).filename().u8string();
	}

	inline void TzSet()
	{
		::_tzset();
	}
}