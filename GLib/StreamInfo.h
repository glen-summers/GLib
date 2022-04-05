#pragma once

#include <GLib/Compat.h>

#include <fstream>

class StreamInfo
{
	std::ofstream mutable stream;
	std::filesystem::path path;
	unsigned int date {};

public:
	StreamInfo(std::ofstream stream, std::filesystem::path path, unsigned int date) noexcept
		: stream(std::move(stream))
		, path(std::move(path))
		, date(date)
	{}

	StreamInfo() = default;
	StreamInfo(const StreamInfo &) = delete;
	StreamInfo(StreamInfo &&) = delete;
	StreamInfo & operator=(const StreamInfo &) = delete;
	StreamInfo & operator=(StreamInfo &&) noexcept = default; // NOLINT(bugprone-exception-escape) ofstream move can throw
	~StreamInfo() = default;

	std::ofstream & Stream() const
	{
		return stream;
	}

	const std::filesystem::path & Path() const
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
