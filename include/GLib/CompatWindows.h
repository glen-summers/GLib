#pragma once

#include "Win/Process.h"

#include <array>
#include <ctime>
#include <filesystem>
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

	inline void StrError(const char * prefix)
	{
		constexpr auto ErrorBufferSize = 256;
		std::array<char, ErrorBufferSize> err{};
		char * msg;
		strerror_s(msg = err.data(), err.size(), errno);
		throw std::logic_error(std::string(prefix)+ " : " + msg);
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
}