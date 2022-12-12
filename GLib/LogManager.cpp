#include "pch.h"

#include "FileLogger.h"

#include <GLib/Compat.h>

using GLib::Flog::LogManager;

std::string LogManager::Unmangle(std::string const & name)
{
	return Compat::Unmangle(name);
}

GLib::Flog::Level LogManager::SetLevel(Level const level)
{
	return FileLogger::SetLogLevel(level);
}

size_t LogManager::SetMaxFileSize(size_t const size)
{
	return FileLogger::SetMaxFileSize(size);
}

void LogManager::SetThreadName(std::string_view const name)
{
	FileLogger::Write(Level::Info, "ThreadName", !name.empty() ? name : "(null)");
	FileLogger::GetLogState().ThreadName(name);
}

std::filesystem::path const & LogManager::GetLogPath()
{
	return FileLogger::Instance().streamInfo.Path();
}
