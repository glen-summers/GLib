#include "pch.h"

#include "filelogger.h"

#include <GLib/flogging.h>

using GLib::Flog::Level;
using GLib::Flog::Log;

void Log::Write(Level level, std::string_view message) const
{
	FileLogger::Write(level, name, message);
}

void Log::ScopeStart(Level level, std::string_view scope, std::string_view stem) const
{
	FileLogger::ScopeStart(level, name, scope, stem);
}

void Log::ScopeEnd() const
{
	FileLogger::ScopeEnd(name);
}

void Log::CommitStream(Level level) const
{
	FileLogger::CommitBuffer(level, name);
}
