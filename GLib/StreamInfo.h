#pragma once

#include <GLib/Compat.h>

#include <fstream>

class StreamInfo
{
	std::ofstream mutable stream;
	GLib::Compat::FileSystem::path path;
	unsigned int date {};

public:
	StreamInfo(std::ofstream stream, GLib::Compat::FileSystem::path path, unsigned int date)
		: stream(std::move(stream))
		, path(std::move(path))
		, date(date)
	{}

	StreamInfo() = default;

	std::ofstream & Stream() const
	{
		return stream;
	}

	const GLib::Compat::FileSystem::path & Path() const
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
