#pragma once

#include <fstream>

class StreamInfo
{
	std::ofstream mutable stream;
	std::string fileName;
	unsigned int date {};

public:
	StreamInfo(std::ofstream stream, std::string fileName, unsigned int date)
		: stream(std::move(stream))
		, fileName(std::move(fileName))
		, date(date)
	{}

	StreamInfo() = default;

	std::ofstream & Stream() const
	{
		return stream;
	}

	const std::string & FileName() const
	{
		return fileName;
	}

	unsigned int Date() const
	{
		return date;
	}

	operator bool() const
	{
		return stream.is_open() && stream.good();
	}
};
