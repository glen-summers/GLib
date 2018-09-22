#pragma once

#include <stdexcept>
#include <ctime>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

#ifdef __linux__
#include <experimental/filesystem>
#elif _WIN32
#include <filesystem>
#else
//?
#endif

namespace GLib
{
	namespace Compat
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

		inline void StrError()
		{
			char err[255];
			char * msg;
#ifdef __linux__
			msg = strerror_r(errno, err, sizeof(err));
#elif _MSC_VER
			strerror_s(msg = err, sizeof(err), errno);
#else
			//?
#endif
			throw std::logic_error(std::string("sprintf error : ") + msg);
		}

		inline std::string Unmangle(const char * name)
		{
#ifdef _MSC_VER
			return ::strncmp(name, "class ", 6) == 0
				? name + 6
				: ::strncmp(name, "struct ", 7) == 0
					? name +7
					: name; // etc...
#elif __GNUG__
			int status = -1;
			std::unique_ptr<char, void(*)(void*)> res { abi::__cxa_demangle(name, NULL, NULL, &status), std::free };
			return status == 0 ? res.get() : name;
#else
			return name;
#endif
		}

	}
}
