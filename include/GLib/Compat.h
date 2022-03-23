#pragma once

#if defined(__linux__) && defined(__GNUG__)
#include "CompatLinux.h"
#elif defined(_WIN32) && defined(_MSC_VER)
#include "CompatWindows.h"
#else
#error unknown
#endif