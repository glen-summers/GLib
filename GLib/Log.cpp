#include "pch.h"

#include "FileLogger.h"

#include <GLib/Flogging.h>

using GLib::Flog::Log;

void Log::Write(Level const level, std::string_view const message) const
{
	FileLogger::Write(level, name, message);
}

void Log::ScopeStart(Level const level, std::string_view const scope, std::string_view const stem) const
{
	FileLogger::ScopeStart(level, name, scope, stem);
}

void Log::ScopeEnd() const
{
	FileLogger::ScopeEnd(name);
}

void Log::CommitStream(Level const level) const
{
	FileLogger::CommitBuffer(level, name);
}
