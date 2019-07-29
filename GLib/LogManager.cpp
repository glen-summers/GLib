#include "pch.h"

#include "GLib/flogging.h"
#include "GLib/compat.h"

#include "filelogger.h"

using GLib::Flog::LogManager;

std::string LogManager::Unmangle(const std::string & name)
{
	return Compat::Unmangle(name);
}

void LogManager::SetLevel(GLib::Flog::Level level)
{
	FileLogger::SetLogLevel(level);
}

void LogManager::SetThreadName(const char * name)
{
	FileLogger::Write(Level::Info, "ThreadName", name != nullptr ? name : "(null)");
	FileLogger::logState.threadName = name;
}

std::string LogManager::GetLogPath()
{
	return FileLogger::Instance().streamInfo.FileName();
}
