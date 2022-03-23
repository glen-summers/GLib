#include "pch.h"

#include "FileLogger.h"

#include <GLib/Compat.h>

using GLib::Flog::LogManager;

std::string LogManager::Unmangle(const std::string & name)
{
	return Compat::Unmangle(name);
}

GLib::Flog::Level LogManager::SetLevel(Level level)
{
	return FileLogger::SetLogLevel(level);
}

size_t LogManager::SetMaxFileSize(size_t size)
{
	return FileLogger::SetMaxFileSize(size);
}

void LogManager::SetThreadName(std::string_view name)
{
	FileLogger::Write(Level::Info, "ThreadName", !name.empty() ? name : "(null)");
	FileLogger::GetLogState().ThreadName(name);
}

GLib::Compat::FileSystem::path LogManager::GetLogPath()
{
	return FileLogger::Instance().streamInfo.Path();
}
