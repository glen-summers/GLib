#include "pch.h"

#include "filelogger.h"

#include <GLib/compat.h>

using GLib::Flog::LogManager;

std::string LogManager::Unmangle(const std::string & name)
{
	return Compat::Unmangle(name);
}

GLib::Flog::Level LogManager::SetLevel(GLib::Flog::Level level)
{
	return FileLogger::SetLogLevel(level);
}

size_t LogManager::SetMaxFileSize(size_t size)
{
	return FileLogger::SetMaxFileSize(size);
}

void LogManager::SetThreadName(const char * name)
{
	FileLogger::Write(Level::Info, "ThreadName", name != nullptr ? name : "(null)");
	FileLogger::logState.ThreadName(name);
}

GLib::Compat::filesystem::path LogManager::GetLogPath()
{
	return FileLogger::Instance().streamInfo.Path();
}
