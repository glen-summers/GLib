#include "pch.h"

#include "GLib/flogging.h"
#include "GLib/compat.h"

#include "filelogger.h"

#include <iomanip>
#include <sstream>
#include <thread>

using namespace GLib;

namespace
{
	std::string GetFullName()
	{
		return Compat::ProcessPath();
		// qt: CoreApplication::applicationFilePath().toStdString();
	}

	std::string GetBaseName()
	{
		return Compat::ProcessName();
		//qt: return fs::path(QCoreApplication::applicationFilePath().toStdString()).filename().u8string();
	}

	unsigned long long GetProcessId()
	{
		return Compat::ProcessId();
		//qt: return QCoreApplication::applicationPid();
	}

	std::thread::id GetThreadId()
	{
		return std::this_thread::get_id();
	}

	template<class Arg> struct Manipulator
	{
		Manipulator(void(*left)(std::ostream&, Arg), Arg val) : p(left), arg(val) {}
		void(*p)(std::ostream&, Arg);
		Arg arg;
	};

	template <typename Arg>
	std::ostream & operator<<(std::ostream& str, const Manipulator<Arg>& m)
	{
		(*m.p)(str, m.arg);
		return str;
	}
}

	thread_local LogState FileLogger::logState;

	FileLogger::FileLogger()
		: baseFileName(GetBaseName() + "_" + std::to_string(GetProcessId()))
		, path(fs::temp_directory_path() / "glogfiles")
	{
		create_directories(path);
	}

	// avoid
	FileLogger::~FileLogger()
	{
		CloseStream(); //
	}

	void FileLogger::Write(Flog::Level level, const char * prefix, const char * message)
	{
		// avoid?
		static FileLogger ftl;
		ftl.InternalWrite(level, prefix, message);
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
		unsigned int date = GetDate();

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

			std::ofstream newStreamWriter;//ctor?
			newStreamWriter.open(logFileName); // FileShare.ReadWrite | FileShare.Delete? HANDLE_FLAG_INHERIT
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
				// ensure we are not tunnelled here...
				// http://blogs.msdn.com/oldnewthing/archive/2005/07/15/439261.aspx
				// http://support.microsoft.com/?kbid=172190
				// FILETIME ft;
				//::GetSystemTimeAsFileTime(&ft);
				//::SetFileTime(GetImpl(m_stream), &ft, NULL, NULL); // filesystem ver? nope

				return StreamInfo { move(newStreamWriter), std::move(logFileName.u8string()), std::move(date) };
			}
			catch (...) // specific
			{
			}
		}
		throw std::runtime_error("Exhausted possible stream names " + logFileName.u8string());
	}

	void FileLogger::InternalWrite(Flog::Level level, const char * prefix, const char *message)
	{
		// ShouldTrace ...
		if (level < logLevel)
		{
			return;
		}

		std::lock_guard<std::mutex> guard(streamMonitor);
		{
			try
			{
				size_t newEntrySize = strlen(message);
				HandleFileRollover(newEntrySize);
				EnsureStreamIsOpen();
				if (!ResourcesAvailable(newEntrySize))
				{
					return;
				}

				// flags etc.
				// move some formatting out of lock

				auto now = std::chrono::system_clock::now();
				std::time_t t = std::chrono::system_clock::to_time_t(now);
				std::tm tm;
				Compat::LocalTime(tm, t);

				int ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

				auto & s = streamInfo.Stream();
				s << std::left
					<< std::put_time(&tm, "%d %b %Y, %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms << std::setfill(' ')
					<< " : [ " << std::setw(THREAD_ID_WIDTH);
				if (logState.threadName)
				{
					s << logState.threadName;
				}
				else
				{
					s << GetThreadId();
				}
				s << " ] : "
					<< std::setw(LEVEL_WIDTH) << Manipulator<Flog::Level>(TranslateLevel, level) << " : "
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
			auto size = streamInfo.Stream().tellp();
			if (newEntrySize + size >= maxFileSize || streamInfo.Date() != GetDate())
			{
				CloseStream();
				//RenameOldFile(oldFile);
			}
		}
	}

	void FileLogger::CloseStream()
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

	void FileLogger::WriteHeader(std::ostream &writer) const
	{
		writer << HeaderFooterSeparator << std::endl;

		std::time_t t = std::time(nullptr);
		std::tm tm;
		Compat::LocalTime(tm, t);
		std::tm gtm;
		Compat::GmTime(gtm, t);

		static constexpr bool is64BitProcess = sizeof(void*) == 8;
		static constexpr int bits = is64BitProcess ? 64 : 32; // more?

		writer
			<< "Opened      : " << std::put_time(&tm, "%d %b %Y, %H:%M:%S (%z)") << std::endl
			<< "OpenedUtc   : " << std::put_time(&gtm, "%F %TZ") << std::endl
			<< "ProcessName : (" << bits << " bit) " << GetBaseName() << std::endl
			<< "FullPath    : " << GetFullName() << std::endl
			<< "ProcessId   : " << GetProcessId() << std::endl
			<< "ThreadId    : " << GetThreadId() << std::endl;
		//Formatter::Format(writer, "UserName    : {0}\\{1}", Environment.UserDomainName, Environment.UserName);

		writer << HeaderFooterSeparator << std::endl;
		writer.flush();
	}

	void FileLogger::WriteFooter(std::ostream &writer)
	{
		std::time_t t = std::time(nullptr);
		std::tm tm;
		Compat::LocalTime(tm, t);

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

void FileLogger::ScopeStart(GLib::Flog::Level level, const char * logName, const char * scope, const char * stem)
{
	if (logState.pendingScope)
	{
		CommitScope(logName);
	}
	logState.scopes.push({ level, scope, stem, std::chrono::high_resolution_clock::now() });
	logState.pendingScope = scope;
}

void FileLogger::Commit(Flog::Level level, const char * name)
{
	Write(level, name, logState.stream.rdbuf().Get());
	logState.stream.rdbuf().Reset(); // had AV here
}

void FileLogger::CommitScope(const char* name)
{
	const Scope & scope = logState.scopes.top();

	std::ostringstream s; // use thread stream
	s << std::setw(logState.depth) << "" << scope.stem << "> " << scope.scope;
	Write(scope.level, name, s.str().c_str());
	++logState.depth;
	logState.pendingScope = nullptr;
}

FileLogger& FileLogger::Instance()
{
	static FileLogger fileLogger;
	return fileLogger;
}

std::ostream & FileLogger::Stream()
{
	return Instance().logState.stream;
}

void FileLogger::SetLogLevel(GLib::Flog::Level level)
{
	Instance().logLevel = level;
}

// use map, use config, set field width
void FileLogger::TranslateLevel(std::ostream & stm, GLib::Flog::Level level)
{
	switch (level)
	{
		case GLib::Flog::Level::Fatal:
			stm << "FATAL   ";
			break;
		case GLib::Flog::Level::Critical:
			stm << "CRITICAL";
			break;
		case GLib::Flog::Level::Error:
			stm << "ERROR   ";
			break;
		case GLib::Flog::Level::Warning:
			stm << "WARNING ";
			break;
		case GLib::Flog::Level::Info:
			stm << "INFO    ";
			break;
		case GLib::Flog::Level::Debug:
			stm << "DEBUG   ";
			break;
		case GLib::Flog::Level::Spam:
			stm << "SPAM    ";
			break;
	}
}

unsigned FileLogger::GetDate()
{
	std::time_t t = std::time(nullptr);
	std::tm tm;
	Compat::LocalTime(tm, t);
	return ((1900 + tm.tm_year) * 100 + tm.tm_mon + 1) * 100 + tm.tm_mday;
}

uintmax_t FileLogger::GetFreeDiskSpace(const fs::path& path)
{
	return space(path).available;
}
