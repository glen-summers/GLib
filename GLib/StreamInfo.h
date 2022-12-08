#pragma once

#include <GLib/Compat.h>

#include <exception>
#include <fstream>

class StreamInfo
{
	std::ofstream mutable stream;
	std::filesystem::path path;
	unsigned int date {};

public:
	StreamInfo(std::ofstream stream, std::filesystem::path path, unsigned int date)
	try
		: stream(std::move(stream))
		, path(std::move(path))
		, date(date)
	{}
	catch (std::exception &)
	{
		std::terminate();
	}

	StreamInfo() = default;
	StreamInfo(StreamInfo const &) = delete;
	StreamInfo(StreamInfo &&) = delete;
	StreamInfo & operator=(StreamInfo const &) = delete;

	StreamInfo & operator=(StreamInfo && s) noexcept
	{
		try
		{
			stream = std::move(s.stream);
			path = std::move(s.path);
			date = s.date;
			return *this;
		}
		catch (std::exception &)
		{
			std::terminate();
		}
	}

	~StreamInfo() = default;

	std::ofstream & Stream() const
	{
		return stream;
	}

	std::filesystem::path const & Path() const
	{
		return path;
	}

	unsigned int Date() const
	{
		return date;
	}

	explicit operator bool() const
	{
		return stream.is_open() && stream.good();
	}
};
