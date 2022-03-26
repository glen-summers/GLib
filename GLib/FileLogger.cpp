#include "pch.h"

#include "FileLogger.h"

#include "DurationPrinter.h"
#include "Manipulator.h"

#include <GLib/Compat.h>

#include <sstream>
#include <thread>

FileLogger::FileLogger()
	: baseFileName(GLib::Compat::ProcessName() + "_" + std::to_string(GLib::Compat::ProcessId()))
	, path(GLib::Compat::FileSystem::temp_directory_path() / "glogfiles")
{
	create_directories(path);
}

FileLogger::~FileLogger()
{
	CloseStream(); //
}

const LogState & FileLogger::GetLogState() noexcept
{
	thread_local LogState const state;
	return state;
}

void FileLogger::Write(GLib::Flog::Level level, std::string_view prefix, std::string_view message)
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
	// static constexpr bool alwaysAddDate = false;

	std::ostringstream s;
	s << baseFileName;
	// if (alwaysAddDate)
	// {
	// 	time_t t = std::time(nullptr);
	// 	tm tm;
	// 	Compat::LocalTime(tm, t);
	// 	s << "_" << std::put_time(&tm, "%Y-%m-%d");
	// }
	GLib::Compat::FileSystem::path logFileName = path / (s.str() + ".log"); // combine, check trailing etc.
	const unsigned int date = GetDate();

	const int MaxTries = 1000;

	// consolidate with renameOldFile??
	for (int num = 0; num < MaxTries; ++num)
	{
		if (num != 0)
		{
			logFileName.replace_filename(s.str() + "_" + std::to_string(num) + ".log");
		}

		// RenameOldFile(logFileName.u8string());

		if (exists(logFileName))
		{
			continue;
		}

		std::ofstream newStreamWriter(logFileName); // FileShare.ReadWrite | FileShare.Delete? HANDLE_FLAG_INHERIT
		if (!newStreamWriter)
		{
			continue;
		}

		// Debug::Write("Flogging to file : {0}", logFileName.u8string());
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
			// FILE-TIME ft;
			//::GetSystemTimeAsFileTime(&ft);
			//::SetFileTime(GetImpl(m_stream), &ft, NULL, NULL); // filesystem ver? nope

			return StreamInfo {move(newStreamWriter), logFileName, date};
		}
		catch (...) // specific
		{}
	}
	throw std::runtime_error("Exhausted possible stream names " + GLib::Cvt::P2A(logFileName));
}

void FileLogger::InternalWrite(GLib::Flog::Level level, std::string_view prefix, std::string_view message)
{
	// ShouldTrace ...
	if (level < logLevel)
	{
		return;
	}

	CommitPendingScope();
	WriteToStream(level, prefix, message);
}

void FileLogger::WriteToStream(GLib::Flog::Level level, std::string_view prefix, std::string_view message)
{
	std::lock_guard guard(streamMonitor);
	{
		try
		{
			const size_t newEntrySize = message.size();
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
			std::tm tm {};
			GLib::Compat::LocalTime(tm, t);

			const auto ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);

			auto & s = streamInfo.Stream();
			s << std::left << std::put_time(&tm, "%d %b %Y, %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms;
			s << std::setfill(' ') << " : [ " << std::setw(threadIdWidth) << Manipulate(ThreadName, logState.ThreadName()) << " ] : ";
			s << std::setw(levelWidth) << Manipulate(TranslateLevel, level) << " : ";
			s << std::setw(prefixWidth) << prefix << " : " << message;
			s << std::endl << std::flush;
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
		// auto oldPath = streamInfo.Path();
		const auto size = streamInfo.Stream().tellp();
		if (newEntrySize + size >= maxFileSize || streamInfo.Date() != GetDate())
		{
			CloseStream();
			// RenameOldFile(oldPath);
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
		{}

		try
		{
			streamInfo.Stream().flush();
		}
		catch (...) // specific?
		{}

		// streamWriter.close();
		streamInfo = StreamInfo();
	}
}

void FileLogger::WriteHeader(std::ostream & writer)
{
	writer << headerFooterSeparator << std::endl;

	const std::time_t t = std::time(nullptr);
	std::tm tm {};
	GLib::Compat::LocalTime(tm, t);
	std::tm gtm {};
	GLib::Compat::GmTime(gtm, t);

	std::string name = GLib::Compat::ProcessName();
	std::string path = GLib::Compat::ProcessPath();
	std::string cmd = GLib::Compat::CommandLine();

	size_t pos = cmd.find(path);
	if (pos != std::string::npos)
	{
		cmd.erase(pos, path.size());
	}

	static constexpr bool is64BitProcess = sizeof(void *) == 8;
	static constexpr int bits = is64BitProcess ? 64 : 32; // more?

	writer << "Opened      : " << std::put_time(&tm, "%d %b %Y, %H:%M:%S (%z)") << std::endl
				 << "OpenedUtc   : " << std::put_time(&gtm, "%F %TZ") << std::endl
				 << "ProcessName : (" << bits << " bit) " << name << std::endl
				 << "FullPath    : " << path << std::endl
				 << "CmdLine     : " << cmd << std::endl
				 << "ProcessId   : " << GLib::Compat::ProcessId() << std::endl
				 << "ThreadId    : " << std::this_thread::get_id() << std::endl;
	// Formatter::Format(writer, "UserName    : {0}\\{1}", Environment.UserDomainName, Environment.UserName);

	writer << headerFooterSeparator << std::endl;
	writer.flush();
}

void FileLogger::WriteFooter(std::ostream & writer)
{
	const std::time_t t = std::time(nullptr);
	std::tm tm {};
	GLib::Compat::LocalTime(tm, t);

	writer << headerFooterSeparator << std::endl
				 << "Closed       " << std::put_time(&tm, "%d %b %Y, %H:%M:%S (%z)") << std::endl
				 << headerFooterSeparator << std::endl;
	writer.flush();
}

bool FileLogger::ResourcesAvailable(size_t newEntrySize) const
{
	return streamInfo && GetFreeDiskSpace(path) - newEntrySize >= reserveDiskSpace;
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
	Instance().WriteToStream(scope.Level(), scope.Prefix(), s.str().c_str());

	logState.Commit();
}

void FileLogger::ScopeStart(GLib::Flog::Level level, std::string_view prefix, std::string_view scope, std::string_view stem)
{
	// level check?
	CommitPendingScope();
	logState.Push({level, prefix, scope, stem});
}

void FileLogger::CommitBuffer(GLib::Flog::Level level, std::string_view prefix)
{
	Write(level, prefix, logState.Get());
	logState.Reset(); // had AV here
}

void FileLogger::ScopeEnd(std::string_view prefix)
{
	auto [scope, pending] = logState.TopAndPop();

	std::ostringstream s; // use thread stream
	s << std::setw(logState.Depth()) << "";
	s << "<" << scope.Stem();

	if (pending)
	{
		s << ">";
	}
	s << " " << scope.ScopeText() << " " << scope.Duration<std::chrono::nanoseconds>();
	Write(scope.Level(), prefix, s.str().c_str());
}

FileLogger & FileLogger::Instance()
{
	static FileLogger fileLogger;
	return fileLogger;
}

std::ostream & FileLogger::Stream()
{
	return logState.Stream();
}

GLib::Flog::Level FileLogger::SetLogLevel(GLib::Flog::Level level)
{
	return std::exchange(Instance().logLevel, level); // atomic?
}

size_t FileLogger::SetMaxFileSize(size_t size)
{
	return std::exchange(Instance().maxFileSize, size); // atomic?
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

std::ostream & FileLogger::ThreadName(std::ostream & stream, std::string_view threadName)
{
	return !threadName.empty() ? stream << threadName : stream << std::this_thread::get_id();
}

unsigned FileLogger::GetDate()
{
	const std::time_t t = std::time(nullptr);
	std::tm tm {};
	GLib::Compat::LocalTime(tm, t);
	constexpr auto tmEpochYear = 1900;
	constexpr auto shiftTwoDecimals = 100;
	return ((tmEpochYear + tm.tm_year) * shiftTwoDecimals + tm.tm_mon + 1) * shiftTwoDecimals + tm.tm_mday;
}

uintmax_t FileLogger::GetFreeDiskSpace(const GLib::Compat::FileSystem::path & path)
{
	return space(path).available;
}