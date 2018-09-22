#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include "streaminfo.h"
#include "logstate.h"

#include "glib/compat.h"

#include <mutex>

namespace fs = GLib::Compat::filesystem;

class FileLogger
{
	static constexpr const char * HeaderFooterSeparator = "------------------------------------------------";
	static constexpr const char * Delimiter = " : ";
	static constexpr int THREAD_ID_WIDTH = 5;
	static constexpr int LEVEL_WIDTH = 8;
	static constexpr int PREFIX_WIDTH = 12;
	static constexpr size_t maxFileSize = 5 * 1024 * 1024;
	static constexpr size_t ReserveDiskSpace = 10 * 1024 * 1024;

	friend class GLib::Flog::Log;
	friend class GLib::Flog::LogManager;

	std::string const baseFileName;
	fs::path const path;
	std::mutex streamMonitor;
	StreamInfo streamInfo;
	GLib::Flog::Level logLevel = GLib::Flog::Level::Info;//
	static thread_local LogState logState;

public:
	FileLogger();
	FileLogger(const FileLogger&) = delete;
	FileLogger(FileLogger&&) = delete;
	FileLogger& operator=(const FileLogger&) = delete;
	FileLogger& operator=(FileLogger&&) = delete;

	static void Write(GLib::Flog::Level level, const char * prefix, const char * message);

private:
	~FileLogger();

	StreamInfo GetStream() const;
	void InternalWrite(GLib::Flog::Level level, const char * prefix, const char * message);
	void EnsureStreamIsOpen();
	void HandleFileRollover(size_t newEntrySize);
	void CloseStream();
	void WriteHeader(std::ostream & writer) const;
	static void WriteFooter(std::ostream & writer);
	bool ResourcesAvailable(size_t newEntrySize) const;
	//std::string RenameOldFile(const std::string & oldFileName) const;

	// improve
	static FileLogger & Instance();
	static std::ostream & Stream();
	static void SetLogLevel(GLib::Flog::Level);
	static void TranslateLevel(std::ostream & stm, GLib::Flog::Level level);
	static unsigned int GetDate();
	static uintmax_t GetFreeDiskSpace(const fs::path & path);

	static void ScopeStart(GLib::Flog::Level level, const char * logName, const char * scope, const char * stem);
	static void Commit(GLib::Flog::Level level, const char * name); // name?
	static void CommitScope(const char * name);
};

#endif // FILE_LOGGER_H
