#pragma once

#include <GLib/Formatter.h>
#include <GLib/GenericOutStream.h>

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
			int_type overflow(int_type const c) override
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

	enum class Level : uint8_t
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
		void Spam(std::string_view const message) const
		{
			Write(Level::Spam, message);
		}

		template <typename... Ts>
		void Spam(std::string_view const format, Ts &&... ts) const
		{
			Write(Level::Spam, format, std::forward<Ts>(ts)...);
		}

		void Debug(std::string_view const message) const
		{
			Write(Level::Debug, message);
		}

		template <typename... Ts>
		void Debug(std::string_view const format, Ts &&... ts) const
		{
			Write(Level::Debug, format, std::forward<Ts>(ts)...);
		}

		void Info(std::string_view const message) const
		{
			Write(Level::Info, message);
		}

		template <typename... Ts>
		void Info(std::string_view const format, Ts &&... ts) const
		{
			Write(Level::Info, format, std::forward<Ts>(ts)...);
		}

		void Warning(std::string_view const message) const
		{
			Write(Level::Warning, message);
		}

		template <typename... Ts>
		void Warning(std::string_view const format, Ts &&... ts) const
		{
			Write(Level::Warning, format, std::forward<Ts>(ts)...);
		}

		void Error(std::string_view const message) const
		{
			Write(Level::Error, message);
		}

		template <typename... Ts>
		void Error(std::string_view const format, Ts &&... ts) const
		{
			Write(Level::Error, format, std::forward<Ts>(ts)...);
		}

		friend class LogManager;
		friend class ScopeLog;

	private:
		explicit Log(std::string name) noexcept
			: name(std::move(name))
		{}

		void Write(Level level, std::string_view message) const;
		void ScopeStart(Level level, std::string_view scope, std::string_view stem) const;
		void ScopeEnd() const;
		// std::ostream & Stream() const;
		void CommitStream(Level level) const;

		template <typename... Ts>
		void Write(Level const level, std::string_view const format, Ts... ts) const
		{
			Formatter::Format(Detail::Stream(), format, std::forward<Ts>(ts)...);
			CommitStream(level);
		}
	};

	class ScopeLog
	{
		Log const & log;
		Level level;
		std::string_view scope;
		std::string_view stem;

	public:
		ScopeLog(Log const & log, Level const level, std::string_view const scope, std::string_view const stem = "==")
			: log(log)
			, level(level)
			, scope(scope)
			, stem(stem)
		{
			log.ScopeStart(this->level, this->scope, this->stem);
		}

		ScopeLog(ScopeLog const &) = delete;
		ScopeLog & operator=(ScopeLog const &) = delete;
		ScopeLog(ScopeLog &&) = default;
		ScopeLog & operator=(ScopeLog &&) = delete;

		~ScopeLog()
		{
			log.ScopeEnd();
		}
	};

	class LogManager
	{
		static std::string Unmangle(std::string const & name);

	public:
		static Level SetLevel(Level level);
		static size_t SetMaxFileSize(size_t size);
		static void SetThreadName(std::string_view name);
		static std::filesystem::path GetLogPath();

		static auto GetLog(std::string const & name) noexcept
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
