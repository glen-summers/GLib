#pragma once

#if defined(_WIN32) && defined(_MSC_VER)

#include <GLib/Cvt.h>
#include <GLib/StackOrHeap.h>
#include <GLib/Win/FileSystem.h>

#include <array>
#include <ctime>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

namespace GLib::Compat
{
	namespace FileSystem = std::filesystem;

	inline void LocalTime(tm & tm, const time_t & t)
	{
		localtime_s(&tm, &t);
	}

	inline void GmTime(tm & tm, const time_t & t)
	{
		gmtime_s(&tm, &t);
	}

	inline void AssertTrue(bool value, const char * prefix, errno_t error)
	{
		if (!value)
		{
			constexpr auto errorBufferSize = 256;
			std::array<char, errorBufferSize> err {};
			strerror_s(err.data(), err.size(), error);
			throw std::runtime_error(std::string(prefix) + " : " + err.data());
		}
	}

	inline void SetEnv(const char * name, const char * value)
	{
		auto wideName = Cvt::A2W(name);
		auto wideValue = Cvt::A2W(value);
		errno_t err = _wputenv_s(wideName.c_str(), wideValue.c_str());
		AssertTrue(err == 0, "_wputenv_s", err);
	}

	inline std::optional<std::string> GetEnv(const char * name)
	{
		auto wideName = Cvt::A2W(name);

		GLib::Util::WideCharBuffer tmp;
		size_t len = 0;
		errno_t err = ::_wgetenv_s(&len, tmp.Get(), tmp.size(), wideName.c_str());
		if (len == 0)
		{
			return {};
		}

		if (len > tmp.size())
		{
			tmp.EnsureSize(len);
			err = _wgetenv_s(&len, tmp.Get(), tmp.size(), wideName.c_str());
		}
		AssertTrue(err == 0, "_wgetenv_s", err);
		return Cvt::W2A({tmp.Get(), len - 1});
	}

	inline void UnsetEnv(const char * name)
	{
		SetEnv(name, "");
	}

	inline std::string Unmangle(const std::string & name)
	{
		constexpr std::string_view Class = "class ";
		constexpr std::string_view Struct = "struct ";

		return name.compare(0, Class.size(), Class) == 0		 ? name.substr(Class.size())
					 : name.compare(0, Struct.size(), Struct) == 0 ? name.substr(Struct.size())
																												 : name; // etc...
	}

	inline int64_t ProcessId()
	{
		return ::GetCurrentProcessId();
	}

	inline std::string ProcessPath()
	{
		return GLib::Win::FileSystem::PathOfModule(nullptr);
	}

	inline std::string ProcessName()
	{
		return Cvt::P2A(FileSystem::path(ProcessPath()).filename());
	}

	inline std::string CommandLine()
	{
		return GLib::Cvt::W2A(::GetCommandLineW());
	}

	inline void TzSet()
	{
		::_tzset();
	}
}

#endif