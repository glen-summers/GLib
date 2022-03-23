#pragma once

#include "LogState.h"
#include "StreamInfo.h"

#include <GLib/Flogging.h>

#include <mutex>

class FileLogger
{
	static constexpr std::string_view HeaderFooterSeparator = "------------------------------------------------";
	static constexpr std::string_view Delimiter = " : ";
	static constexpr int THREAD_ID_WIDTH = 5; // make dynamic
	static constexpr int LEVEL_WIDTH = 8;
	static constexpr int PREFIX_WIDTH = 16; // make dynamic
	static constexpr size_t DefaultMaxFileSize = 5 * 1024 * 1024;
	static constexpr size_t ReserveDiskSpace = 10 * 1024 * 1024;

	friend class GLib::Flog::Log;
	friend class GLib::Flog::LogManager;

	std::string const baseFileName;
	GLib::Compat::filesystem::path const path;
	std::mutex streamMonitor;
	StreamInfo streamInfo;
	GLib::Flog::Level logLevel = GLib::Flog::Level::Info; // config
	size_t maxFileSize = DefaultMaxFileSize;							// config

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
	static uintmax_t GetFreeDiskSpace(const GLib::Compat::filesystem::path & path);

	static void CommitPendingScope();
	static void ScopeStart(GLib::Flog::Level level, std::string_view prefix, std::string_view scope, std::string_view stem);
	static void CommitBuffer(GLib::Flog::Level level, std::string_view prefix);
	static void ScopeEnd(std::string_view prefix);
};
