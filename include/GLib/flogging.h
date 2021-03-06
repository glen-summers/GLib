#ifndef FLOGGING_H
#define FLOGGING_H

#include <GLib/formatter.h>
#include <GLib/genericoutstream.h>

#include <string>
#include <utility>

namespace GLib::Flog
{
	namespace Detail
	{
		extern "C" void Write(char c);

		// perf test
		class Buffer : public std::basic_streambuf<char>
		{
		protected:
			int_type overflow(int_type c) override
			{
				if (traits_type::eq_int_type(c, traits_type::eof()))
				{
					return traits_type::not_eof(c);
				}
				Write(traits_type::to_char_type(c));
				return c;
			}
		};
		using StreamType = Util::GenericOutStream<char, Buffer>;

		inline std::ostream & Stream()
		{
			static StreamType stream;
			return stream.Stream();
		}
	}

	enum class Level : unsigned
	{
		Spam,
		Debug,
		Info,
		Warning,
		Error,
		Critical,
		Fatal
	};

	class LogManager;
	class ScopeLog;

	class Log
	{
		std::string const name;

	public:
		void Spam(const char * message) const
		{
			Write(Level::Spam, message);
		}

		void Spam(const std::string & message) const
		{
			Write(Level::Spam, message.c_str());
		}

		template <typename... Ts>
		void Spam(const char * format, Ts &&... ts) const
		{
			Write(Level::Spam, format, std::forward<Ts>(ts)...);
		}

		void Debug(const char * message) const
		{
			Write(Level::Debug, message);
		}

		void Debug(const std::string & message) const
		{
			Write(Level::Debug, message.c_str());
		}

		template <typename... Ts>
		void Debug(const char * format, Ts &&... ts) const
		{
			Write(Level::Debug, format, std::forward<Ts>(ts)...);
		}

		void Info(const char * message) const
		{
			Write(Level::Info, message);
		}

		void Info(const std::string & message) const
		{
			Write(Level::Info, message.c_str());
		}

		template <typename... Ts>
		void Info(const char * format, Ts &&... ts) const
		{
			Write(Level::Info, format, std::forward<Ts>(ts)...);
		}

		void Warning(const char * message) const
		{
			Write(Level::Warning, message);
		}

		void Warning(const std::string & message) const
		{
			Write(Level::Warning, message.c_str());
		}

		template <typename... Ts>
		void Warning(const char * format, Ts &&... ts) const
		{
			Write(Level::Warning, format, std::forward<Ts>(ts)...);
		}

		void Error(const char * message) const
		{
			Write(Level::Error, message);
		}

		void Error(const std::string & message) const
		{
			Write(Level::Error, message.c_str());
		}

		template <typename... Ts>
		void Error(const char * format, Ts &&... ts) const
		{
			Write(Level::Error, format, std::forward<Ts>(ts)...);
		}

		friend class LogManager;
		friend class ScopeLog;

	private:
		Log(std::string name) noexcept
			: name(std::move(name))
		{}

		void Write(Level level, const char * message) const;
		void ScopeStart(Level level, const char * scope, const char * stem) const;
		void ScopeEnd() const;
		// std::ostream & Stream() const;
		void CommitStream(Level level) const;

		template <typename... Ts>
		void Write(Level level, const char * format, Ts... ts) const
		{
			Formatter::Format(Detail::Stream(), format, std::forward<Ts>(ts)...);
			CommitStream(level);
		}
	};

	class ScopeLog
	{
		const Log & log;
		Level level;
		const char * scope;
		const char * stem;

	public:
		ScopeLog(const ScopeLog &) = delete;
		ScopeLog & operator=(const ScopeLog &) = delete;
		ScopeLog(ScopeLog &&) = default;
		ScopeLog & operator=(ScopeLog &&) = delete;

		ScopeLog(const Log & log, Level level, const char * scope, const char * stem = "==")
			: log(log)
			, level(level)
			, scope(scope)
			, stem(stem)
		{
			log.ScopeStart(this->level, this->scope, this->stem);
		}

		~ScopeLog()
		{
			log.ScopeEnd();
		}
	};

	class LogManager
	{
		static std::string Unmangle(const std::string & name);

	public:
		static Level SetLevel(Level level);
		static size_t SetMaxFileSize(size_t size);
		static void SetThreadName(const char * name);
		static GLib::Compat::filesystem::path GetLogPath();

		static Log GetLog(const std::string & name) noexcept
		{
			return Log(name);
		}

		template <typename T>
		static Log GetLog() noexcept
		{
			return Log(Unmangle(typeid(T).name()));
		}
	};
}
#endif // FLOGGING_H
