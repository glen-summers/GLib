#pragma once

#include "LogState.h"
#include "StreamInfo.h"

#include <GLib/Flogging.h>

#include <mutex>

class FileLogger
{
	static constexpr std::string_view headerFooterSeparator = "------------------------------------------------";
	static constexpr std::string_view delimiter = " : ";
	static constexpr int threadIdWidth = 5; // make dynamic
	static constexpr int levelWidth = 8;
	static constexpr int prefixWidth = 16; // make dynamic
	static constexpr auto defaultMaxFileSize = size_t {5} * 1024 * 1024;
	static constexpr auto reserveDiskSpace = size_t {10} * 1024 * 1024;

	friend class GLib::Flog::Log;
	friend class GLib::Flog::LogManager;

	std::string const baseFileName;
	GLib::Compat::FileSystem::path const path;
	std::mutex streamMonitor;
	StreamInfo streamInfo;
	GLib::Flog::Level logLevel = GLib::Flog::Level::Info; // config
	size_t maxFileSize = defaultMaxFileSize;							// config

public:
	FileLogger();
	FileLogger(const FileLogger &) = delete;
	FileLogger(FileLogger &&) = delete;
	FileLogger & operator=(const FileLogger &) = delete;
	FileLogger & operator=(FileLogger &&) = delete;

	static void Write(char c);

private:
	~FileLogger();

	static const LogState & GetLogState() noexcept;
	static inline const LogState & logState {GetLogState()};

	static void Write(GLib::Flog::Level level, std::string_view prefix, std::string_view message);

	StreamInfo GetStream() const;
	void InternalWrite(GLib::Flog::Level level, std::string_view prefix, std::string_view message);
	void WriteToStream(GLib::Flog::Level level, std::string_view prefix, std::string_view message);
	void EnsureStreamIsOpen();
	void HandleFileRollover(size_t newEntrySize);
	void CloseStream() noexcept;
	static void WriteHeader(std::ostream & writer);
	static void WriteFooter(std::ostream & writer);
	bool ResourcesAvailable(size_t newEntrySize) const;
	// std::string RenameOldFile(const std::string & oldFileName) const;

	// improve
	static FileLogger & Instance();
	static std::ostream & Stream();
	static GLib::Flog::Level SetLogLevel(GLib::Flog::Level level);
	static size_t SetMaxFileSize(size_t size);
	static std::ostream & TranslateLevel(std::ostream & stream, GLib::Flog::Level level);
	static std::ostream & ThreadName(std::ostream & stream, std::string_view threadName);
	static unsigned int GetDate();
	static uintmax_t GetFreeDiskSpace(const GLib::Compat::FileSystem::path & path);

	static void CommitPendingScope();
	static void ScopeStart(GLib::Flog::Level level, std::string_view prefix, std::string_view scope, std::string_view stem);
	static void CommitBuffer(GLib::Flog::Level level, std::string_view prefix);
	static void ScopeEnd(std::string_view prefix);
};
