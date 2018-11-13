#pragma once

#include <ostream>

template <typename T, class BufferType>
class GenericOutStream : public std::basic_ostream<T>
{
	typedef std::basic_ostream<T> base;

	BufferType buf;

public:
	GenericOutStream(std::ios_base::fmtflags f = std::ios_base::fmtflags()) : base(&buf)
	{
		base::setf(f);
	}

	const BufferType & rdbuf()
	{
		return buf;
	}
};

