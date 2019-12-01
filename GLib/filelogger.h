#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include "logstate.h"
#include "streaminfo.h"

#include <GLib/flogging.h>

#include <mutex>

class FileLogger
{
	static constexpr const char * HeaderFooterSeparator = "------------------------------------------------";
	static constexpr const char * Delimiter = " : ";
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
	size_t maxFileSize = DefaultMaxFileSize; // config
	static thread_local LogState logState;

public:
	FileLogger();
	FileLogger(const FileLogger&) = delete;
	FileLogger(FileLogger&&) = delete;
	FileLogger& operator=(const FileLogger&) = delete;
	FileLogger& operator=(FileLogger&&) = delete;

	static void Write(char c);

private:
	static void Write(GLib::Flog::Level level, const char * prefix, std::string_view message);
	~FileLogger();

	StreamInfo GetStream() const;
	void InternalWrite(GLib::Flog::Level level, const char * prefix, std::string_view message);
	void WriteToStream(GLib::Flog::Level level, const char * prefix, std::string_view message);
	void EnsureStreamIsOpen();
	void HandleFileRollover(size_t newEntrySize);
	void CloseStream() noexcept;
	static void WriteHeader(std::ostream & writer);
	static void WriteFooter(std::ostream & writer);
	bool ResourcesAvailable(size_t newEntrySize) const;
	//std::string RenameOldFile(const std::string & oldFileName) const;

	// improve
	static FileLogger & Instance();
	static std::ostream & Stream();
	static GLib::Flog::Level SetLogLevel(GLib::Flog::Level level);
	static size_t SetMaxFileSize(size_t size);
	static std::ostream & TranslateLevel(std::ostream & stream, GLib::Flog::Level level);
	static std::ostream & ThreadName(std::ostream & stream, const char * threadName);
	static unsigned int GetDate();
	static uintmax_t GetFreeDiskSpace(const GLib::Compat::filesystem::path & path);

	static void CommitPendingScope();
	static void ScopeStart(GLib::Flog::Level level, const char * prefix, const char * scope, const char * stem);
	static void CommitBuffer(GLib::Flog::Level level, const char * prefix);
	static void ScopeEnd(const char * prefix);
};

#endif // FILE_LOGGER_H
