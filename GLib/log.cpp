#include "stdafx.h"

#include "GLib/flogging.h"
#include "filelogger.h"

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
	FileLogger::CommitScope(name.c_str());
}

std::ostream & Log::Stream() const
{
	return FileLogger::Stream();
}

void Log::CommitStream(Level level) const
{
	FileLogger::Commit(level, name.c_str());
}
