#include "pch.h"

#include "FileLogger.h"

#include "DurationPrinter.h"
#include "Manipulator.h"

#include <GLib/Compat.h>

#include <sstream>
#include <thread>

FileLogger::FileLogger()
	: baseFileName(GLib::Compat::ProcessName() + "_" + std::to_string(GLib::Compat::ProcessId()))
	, path(std::filesystem::temp_directory_path() / "glogfiles")
{
	create_directories(path);
}

FileLogger::~FileLogger()
{
	CloseStream(); //
}

LogState const & FileLogger::GetLogState() noexcept
{
	try
	{
		thread_local LogState const state;
		return state;
	}
	catch (std::exception &)
	{
		std::terminate();
	}
}

void FileLogger::Write(GLib::Flog::Level const level, std::string_view const prefix, std::string_view const message)
{
	Instance().InternalWrite(level, prefix, message);
}

void FileLogger::Write(char const chr)
{
	logState.Put(chr);
}

extern "C" void GLib::Flog::Detail::Write(char const chr)
{
	FileLogger::Write(chr);
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

	// if (alwaysAddDate)
	// {
	// 	time_t t = std::time(nullptr);
	// 	tm tm;
	// 	Compat::LocalTime(tm, t);
	// 	s << "_" << std::put_time(&tm, "%Y-%m-%d");
	// }
	std::filesystem::path baseLogFileName = (path / baseFileName).replace_extension(".log");
	std::filesystem::path logFileName;
	// combine, check trailing etc.
	unsigned int const date = GetDate();

	constexpr int maxTries = 1000;

	// consolidate with renameOldFile??
	for (int num = 0; num < maxTries; ++num)
	{
		if (num != 0)
		{
			logFileName = baseLogFileName.replace_filename(baseFileName + "_" + std::to_string(num));
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

			return {std::move(newStreamWriter), std::move(logFileName), date};
		}
		catch (std::exception &)
		{}
	}
	throw std::runtime_error("Exhausted possible stream names " + GLib::Cvt::P2A(logFileName));
}

void FileLogger::InternalWrite(GLib::Flog::Level const level, std::string_view const prefix, std::string_view const message)
{
	// ShouldTrace ...
	if (level < logLevel)
	{
		return;
	}

	CommitPendingScope();
	WriteToStream(level, prefix, message);
}

void FileLogger::WriteToStream(GLib::Flog::Level const level, std::string_view const prefix, std::string_view const message)
{
	std::lock_guard const guard(streamMonitor);
	{
		try
		{
			size_t const newEntrySize = message.size();
			HandleFileRollover(newEntrySize);
			EnsureStreamIsOpen();
			if (!ResourcesAvailable(newEntrySize))
			{
				return;
			}

			// flags etc.
			// move some formatting out of lock

			auto const now = std::chrono::system_clock::now();
			std::time_t const time = std::chrono::system_clock::to_time_t(now);
			std::tm tmValue {};
			GLib::Compat::LocalTime(tmValue, time);

			auto const millisecs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000);

			auto & stm = streamInfo.Stream();
			stm << std::left << std::put_time(&tmValue, "%d %b %Y, %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << millisecs;
			stm << std::setfill(' ') << " : [ " << std::setw(threadIdWidth) << Manipulate(ThreadName, logState.ThreadName()) << " ] : ";
			stm << std::setw(levelWidth) << Manipulate(TranslateLevel, level) << " : ";
			stm << std::setw(prefixWidth) << prefix << " : " << message;
			stm << std::endl << std::flush;
		}
		catch (std::exception &)
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

void FileLogger::HandleFileRollover(size_t const newEntrySize)
{
	if (streamInfo)
	{
		// auto oldPath = streamInfo.Path();
		auto const size = streamInfo.Stream().tellp();
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
		catch (std::exception &)
		{}

		try
		{
			streamInfo.Stream().flush();
		}
		catch (std::exception &)
		{}

		try
		{
			streamInfo = StreamInfo();
		}
		catch (std::exception &)
		{}
	}
}

void FileLogger::WriteHeader(std::ostream & writer)
{
	writer << headerFooterSeparator << std::endl;

	std::time_t const time = std::time(nullptr);
	std::tm tmValue {};
	GLib::Compat::LocalTime(tmValue, time);
	std::tm gtm {};
	GLib::Compat::GmTime(gtm, time);

	std::string const name = GLib::Compat::ProcessName();
	std::string const path = GLib::Compat::ProcessPath();
	std::string cmd = GLib::Compat::CommandLine();

	size_t const pos = cmd.find(path);
	if (pos != std::string::npos)
	{
		cmd.erase(pos, path.size());
	}

	static constexpr bool is64BitProcess = sizeof(void *) == 8;
	static constexpr int bits = is64BitProcess ? 64 : 32; // more?

	writer << "Opened      : " << std::put_time(&tmValue, "%d %b %Y, %H:%M:%S (%z)") << std::endl
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
	std::time_t const time = std::time(nullptr);
	std::tm tmValue {};
	GLib::Compat::LocalTime(tmValue, time);

	writer << headerFooterSeparator << std::endl
				 << "Closed       " << std::put_time(&tmValue, "%d %b %Y, %H:%M:%S (%z)") << std::endl
				 << headerFooterSeparator << std::endl;
	writer.flush();
}

bool FileLogger::ResourcesAvailable(size_t const newEntrySize) const
{
	return streamInfo && GetFreeDiskSpace(path) - newEntrySize >= reserveDiskSpace;
}

void FileLogger::CommitPendingScope()
{
	if (!logState.Pending())
	{
		return;
	}

	Scope const & scope = logState.Top();
	std::ostringstream stm; // use thread stream
	stm << std::setw(logState.Depth()) << "" << scope.Stem() << "> " << scope.ScopeText();

	// need to go via Instance() again as method is static due to use of logState
	Instance().WriteToStream(scope.Level(), scope.Prefix(), stm.str().c_str());

	logState.Commit();
}

void FileLogger::ScopeStart(GLib::Flog::Level level, std::string_view prefix, std::string_view scope, std::string_view stem)
{
	// level check?
	CommitPendingScope();
	logState.Push({level, prefix, scope, stem});
}

void FileLogger::CommitBuffer(GLib::Flog::Level const level, std::string_view const prefix)
{
	Write(level, prefix, logState.Get());
	logState.Reset(); // had AV here
}

void FileLogger::ScopeEnd(std::string_view const prefix)
{
	auto [scope, pending] = logState.TopAndPop();

	std::ostringstream stm; // use thread stream
	stm << std::setw(logState.Depth()) << "";
	stm << "<" << scope.Stem();

	if (pending)
	{
		stm << ">";
	}
	stm << " " << scope.ScopeText() << " " << scope.Duration<std::chrono::nanoseconds>();
	Write(scope.Level(), prefix, stm.str().c_str());
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
std::ostream & FileLogger::TranslateLevel(std::ostream & stream, GLib::Flog::Level const level)
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

std::ostream & FileLogger::ThreadName(std::ostream & stream, std::string_view const threadName)
{
	return !threadName.empty() ? stream << threadName : stream << std::this_thread::get_id();
}

unsigned FileLogger::GetDate()
{
	std::time_t const time = std::time(nullptr);
	std::tm tmValue {};
	GLib::Compat::LocalTime(tmValue, time);
	constexpr auto tmEpochYear = 1900;
	constexpr auto shiftTwoDecimals = 100;
	return ((tmEpochYear + tmValue.tm_year) * shiftTwoDecimals + tmValue.tm_mon + 1) * shiftTwoDecimals + tmValue.tm_mday;
}

uintmax_t FileLogger::GetFreeDiskSpace(std::filesystem::path const & path)
{
	return space(path).available;
}