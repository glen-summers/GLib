#include "pch.h"

#include "filelogger.h"

#include "DurationPrinter.h"
#include "Manipulator.h"

#include "GLib/compat.h"
#include "GLib/flogging.h"

#include <sstream>
#include <thread>

thread_local LogState FileLogger::logState;

FileLogger::FileLogger()
	: baseFileName(GLib::Compat::ProcessName() + "_" + std::to_string(GLib::Compat::ProcessId()))
	, path(fs::temp_directory_path() / "glogfiles")
{
	create_directories(path);
}

// avoid
FileLogger::~FileLogger()
{
	CloseStream(); //
}

void FileLogger::Write(GLib::Flog::Level level, const char * prefix, const char * message)
{
	Instance().InternalWrite(level, prefix, message);
}

void FileLogger::Write(char c)
{
	logState.Put(c);
}

extern "C" void GLib::Flog::Detail::Write(char c)
{
	FileLogger::Write(c);
}

StreamInfo FileLogger::GetStream() const
{
	// the reason to add yyyy-MM-dd at the onset is so that the file collision rate is lower in that it wont hit a random old file
	// and we're not renaming old files here atm (which has the file time tunneling problem http://support2.microsoft.com/kb/172190)
	// with date added at start there is no need to add at file rollover time, and no need to rename the file at all
	// however flogTail currently does not work with this format
	// without adding the date at start, it currently wont get added at rollover
	// rollover will just increase the fileCount app_pid_n, and again will confuse flog tails fileChain logic
	// change this to be full flog compliant, with some params, then upgrade flog tail to handle any scenario
	//static constexpr bool alwaysAddDate = false;

	std::ostringstream s;
	s << baseFileName;
	// if (alwaysAddDate)
	// {
	// 	time_t t = std::time(nullptr);
	// 	tm tm;
	// 	Compat::LocalTime(tm, t);
	// 	s << "_" << std::put_time(&tm, "%Y-%m-%d");
	// }
	fs::path logFileName = path / (s.str() + ".log"); // combine, check trailing etc.
	const unsigned int date = GetDate();

	const int MaxTries = 1000;

	// consolidate with renameOldFile??
	for (int num = 0; num < MaxTries; ++num)
	{
		if (num != 0)
		{
			logFileName.replace_filename(s.str() + "_" + std::to_string(num) + ".log");
		}

		//RenameOldFile(logFileName.u8string());

		if (exists(logFileName))
		{
			continue;
		}

		std::ofstream newStreamWriter(logFileName); // FileShare.ReadWrite | FileShare.Delete? HANDLE_FLAG_INHERIT
		if (!newStreamWriter)
		{
			continue;
		}

		//Debug::Write("Flogging to file : {0}", logFileName.u8string());
		try
		{
			//					Stream stream = File.Open(str, FileMode.CreateNew, FileAccess.Write, FileShare.ReadWrite | FileShare.Delete);
			//					newStreamWriter = new StreamWriter(stream, encoding){ AutoFlush = true };
			WriteHeader(newStreamWriter);

			// rolled over from file...
			// flush?
			// ensure we are not tunneled here...
			// http://blogs.msdn.com/oldnewthing/archive/2005/07/15/439261.aspx
			// http://support.microsoft.com/?kbid=172190
			// FILETIME ft;
			//::GetSystemTimeAsFileTime(&ft);
			//::SetFileTime(GetImpl(m_stream), &ft, NULL, NULL); // filesystem ver? nope

			return StreamInfo { move(newStreamWriter), logFileName.u8string(), date };
		}
		catch (...) // specific
		{
		}
	}
	throw std::runtime_error("Exhausted possible stream names " + logFileName.u8string());
}

void FileLogger::InternalWrite(GLib::Flog::Level level, const char * prefix, const char * message)
{
	// ShouldTrace ...
	if (level < logLevel)
	{
		return;
	}

	CommitPendingScope();
	WriteToStream(level, prefix, message);
}

void FileLogger::WriteToStream(GLib::Flog::Level level, const char * prefix, const char * message)
{
	std::lock_guard<std::mutex> guard(streamMonitor);
	{
		try
		{
			const size_t newEntrySize = strlen(message);
			HandleFileRollover(newEntrySize);
			EnsureStreamIsOpen();
			if (!ResourcesAvailable(newEntrySize))
			{
				return;
			}

			// flags etc.
			// move some formatting out of lock

			auto now = std::chrono::system_clock::now();
			const std::time_t t = std::chrono::system_clock::to_time_t(now);
			std::tm tm{};
			GLib::Compat::LocalTime(tm, t);

			const auto ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);

			streamInfo.Stream() << std::left
				<< std::put_time(&tm, "%d %b %Y, %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms << std::setfill(' ')
				<< " : [ " << std::setw(THREAD_ID_WIDTH) << Manipulate(ThreadName, logState.ThreadName()) << " ] : "
				<< std::setw(LEVEL_WIDTH) << Manipulate(TranslateLevel, level) << " : "
				<< std::setw(PREFIX_WIDTH) << prefix << " : "
				<< message << std::endl << std::flush;
		}
		catch (...)
		{
			CloseStream();
			throw;
		}
	}
}

void FileLogger::EnsureStreamIsOpen()
{
	if (!streamInfo)
	{
		streamInfo = GetStream();
	}
}

void FileLogger::HandleFileRollover(size_t newEntrySize)
{
	if (streamInfo)
	{
		std::string oldFile = streamInfo.FileName();
		const auto size = streamInfo.Stream().tellp();
		if (newEntrySize + size >= maxFileSize || streamInfo.Date() != GetDate())
		{
			CloseStream();
			//RenameOldFile(oldFile);
		}
	}
}

void FileLogger::CloseStream() noexcept
{
	if (streamInfo)
	{
		try
		{
			WriteFooter(streamInfo.Stream());
		}
		catch (...) // specific?
		{
		}
		try
		{
			streamInfo.Stream().flush();
		}
		catch (...) // specific?
		{
		}
		//streamWriter.close();
		streamInfo = StreamInfo();
	}
}

void FileLogger::WriteHeader(std::ostream &writer)
{
	writer << HeaderFooterSeparator << std::endl;

	const std::time_t t = std::time(nullptr);
	std::tm tm {};
	GLib::Compat::LocalTime(tm, t);
	std::tm gtm {};
	GLib::Compat::GmTime(gtm, t);

	static constexpr bool is64BitProcess = sizeof(void*) == 8;
	static constexpr int bits = is64BitProcess ? 64 : 32; // more?

	writer
		<< "Opened      : " << std::put_time(&tm, "%d %b %Y, %H:%M:%S (%z)") << std::endl
		<< "OpenedUtc   : " << std::put_time(&gtm, "%F %TZ") << std::endl
		<< "ProcessName : (" << bits << " bit) " << GLib::Compat::ProcessName() << std::endl
		<< "FullPath    : " << GLib::Compat::ProcessPath() << std::endl
		<< "ProcessId   : " << GLib::Compat::ProcessId() << std::endl
		<< "ThreadId    : " << std::this_thread::get_id() << std::endl;
	//Formatter::Format(writer, "UserName    : {0}\\{1}", Environment.UserDomainName, Environment.UserName);

	writer << HeaderFooterSeparator << std::endl;
	writer.flush();
}

void FileLogger::WriteFooter(std::ostream &writer)
{
	const std::time_t t = std::time(nullptr);
	std::tm tm {};
	GLib::Compat::LocalTime(tm, t);

	writer
		<< HeaderFooterSeparator << std::endl
		<< "Closed       " << std::put_time(&tm, "%d %b %Y, %H:%M:%S (%z)") << std::endl
		<< HeaderFooterSeparator << std::endl;
	writer.flush();
}

bool FileLogger::ResourcesAvailable(size_t newEntrySize) const
{
	// called after rollover check
	// if newEntrySize large and just caused a rollover, and new+header > MaxFileSize then it will not get logged!
	return streamInfo && (newEntrySize + streamInfo.Stream().tellp() <= maxFileSize)
		&& GetFreeDiskSpace(path) - newEntrySize >= ReserveDiskSpace;
}

void FileLogger::CommitPendingScope()
{
	if (!logState.Pending())
	{
		return;
	}

	const Scope & scope = logState.Top();
	std::ostringstream s; // use thread stream
	s << std::setw(logState.Depth()) << "" << scope.Stem() << "> " << scope.ScopeText();

	// need to go via Instance() again as method is static due to use of logState
	Instance().WriteToStream(scope.Level(), scope.Prefix().c_str(), s.str().c_str());

	logState.Commit();
}

void FileLogger::ScopeStart(GLib::Flog::Level level, const char * prefix, const char * scope, const char * stem)
{
	// level check?
	CommitPendingScope();
	logState.Push({ level, prefix, scope, stem });
}

void FileLogger::CommitBuffer(GLib::Flog::Level level, const char * prefix)
{
	Write(level, prefix, logState.Get());
	logState.Reset(); // had AV here
}

void FileLogger::ScopeEnd(const char * prefix)
{
	const Scope scope = logState.Top();
	bool pending = logState.Pop();

	std::ostringstream s; // use thread stream
	s << std::setw(logState.Depth()) << "" << "<" << scope.Stem();

	if (pending)
	{
		s << ">";
	}
	s << " " << scope.ScopeText() << " " << scope.Duration<std::chrono::nanoseconds>();
	Write(scope.Level(), prefix, s.str().c_str());
}

FileLogger& FileLogger::Instance()
{
	static FileLogger fileLogger;
	return fileLogger;
}

std::ostream & FileLogger::Stream()
{
	return Instance().logState.Stream();
}

void FileLogger::SetLogLevel(GLib::Flog::Level level)
{
	Instance().logLevel = level;
}

// use map, use config, set field width
std::ostream & FileLogger::TranslateLevel(std::ostream & stream, GLib::Flog::Level level)
{
	switch (level)
	{
		case GLib::Flog::Level::Fatal:
			stream << "FATAL   ";
			break;
		case GLib::Flog::Level::Critical:
			stream << "CRITICAL";
			break;
		case GLib::Flog::Level::Error:
			stream << "ERROR   ";
			break;
		case GLib::Flog::Level::Warning:
			stream << "WARNING ";
			break;
		case GLib::Flog::Level::Info:
			stream << "INFO    ";
			break;
		case GLib::Flog::Level::Debug:
			stream << "DEBUG   ";
			break;
		case GLib::Flog::Level::Spam:
			stream << "SPAM    ";
			break;
	}
	return stream;
}

std:: ostream & FileLogger::ThreadName(std:: ostream & stream, const char * threadName)
{
	return threadName != nullptr
		? stream << threadName
		: stream << std::this_thread::get_id();
}

unsigned FileLogger::GetDate()
{
	const std::time_t t = std::time(nullptr);
	std::tm tm {};
	GLib::Compat::LocalTime(tm, t);
	constexpr auto TmEpochYear = 1900;
	constexpr auto ShiftTwoDecimals = 100;
	return ((TmEpochYear + tm.tm_year) * ShiftTwoDecimals + tm.tm_mon + 1) * ShiftTwoDecimals + tm.tm_mday;
}

uintmax_t FileLogger::GetFreeDiskSpace(const fs::path& path)
{
	return space(path).available;
}