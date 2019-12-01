#include "pch.h"

#include "filelogger.h"

#include <GLib/flogging.h>

using GLib::Flog::Log;
using GLib::Flog::Level;

void Log::Write(Level level, const char * message) const
{
	FileLogger::Write(level, name.c_str(), message);
}

void Log::ScopeStart(Level level, const char * scope, const char * stem) const
{
	FileLogger::ScopeStart(level, name.c_str(), scope, stem);
}

void Log::ScopeEnd() const
{
	FileLogger::ScopeEnd(name.c_str());
}

void Log::CommitStream(Level level) const
{
	FileLogger::CommitBuffer(level, name.c_str());
}
