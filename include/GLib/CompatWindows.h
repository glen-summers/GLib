#pragma once

#if defined(_WIN32) && defined(_MSC_VER)

#include <GLib/Cvt.h>
#include <GLib/StackOrHeap.h>
#include <GLib/Win/FileSystem.h>

#include <ctime>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

namespace GLib::Compat
{
	inline void ThrowErrorNumber(std::string_view const prefix, errno_t const errorNumber)
	{
		if (errorNumber != 0)
		{
			Util::CharBuffer soh;
			while (strerror_s(soh.Get(), soh.Size(), errorNumber) != 0)
			{
				soh.EnsureSize(soh.Size() * 2);
			}

			if (strerror_s(soh.Get(), soh.Size(), errorNumber) != 0)
			{
				throw std::runtime_error {"strerror_s Failed"};
			}
			throw std::runtime_error {std::string {prefix} + " : " + soh.Get()};
		}
	}

	inline void LocalTime(tm & tm, time_t const & t)
	{
		ThrowErrorNumber(static_cast<char const *>(__func__), localtime_s(&tm, &t));
	}

	inline void GmTime(tm & tm, time_t const & t)
	{
		ThrowErrorNumber(static_cast<char const *>(__func__), gmtime_s(&tm, &t));
	}

	inline void AssertTrue(bool const value, std::string_view const prefix, errno_t const error)
	{
		if (!value)
		{
			ThrowErrorNumber(prefix, error);
		}
	}

	inline void SetEnv(char const * name, char const * value)
	{
		auto const wideName = Cvt::A2W(name);
		auto const wideValue = Cvt::A2W(value);
		errno_t const err = _wputenv_s(wideName.c_str(), wideValue.c_str());
		AssertTrue(err == 0, "_wputenv_s", err);
	}

	inline std::optional<std::string> GetEnv(char const * name)
	{
		auto const wideName = Cvt::A2W(name);

		Util::WideCharBuffer tmp;
		size_t len = 0;
		errno_t err = _wgetenv_s(&len, tmp.Get(), tmp.Size(), wideName.c_str());
		if (len == 0)
		{
			return {};
		}

		if (len > tmp.Size())
		{
			tmp.EnsureSize(len);
			err = _wgetenv_s(&len, tmp.Get(), tmp.Size(), wideName.c_str());
		}
		AssertTrue(err == 0, "_wgetenv_s", err);
		return Cvt::W2A({tmp.Get(), len - 1});
	}

	inline void UnsetEnv(char const * name)
	{
		SetEnv(name, "");
	}

	inline std::string Unmangle(std::string const & name)
	{
		constexpr std::string_view Class = "class ";
		constexpr std::string_view Struct = "struct ";

		return name.compare(0, Class.size(), Class) == 0		 ? name.substr(Class.size())
					 : name.compare(0, Struct.size(), Struct) == 0 ? name.substr(Struct.size())
																												 : name; // etc...
	}

	inline int64_t ProcessId()
	{
		return GetCurrentProcessId();
	}

	inline std::string ProcessPath()
	{
		return Win::FileSystem::PathOfModule(nullptr);
	}

	inline std::string ProcessName()
	{
		return Cvt::P2A(std::filesystem::path(ProcessPath()).filename());
	}

	inline std::string CommandLine()
	{
		return Cvt::W2A(GetCommandLineW());
	}

	inline void TzSet()
	{
		_tzset();
	}
}

#endif