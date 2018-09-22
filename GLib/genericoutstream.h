#pragma once

#include <ostream>

template <typename T, class BufferType> // BufferType : basic_streambuf<_Elem, _Traits>
class GenericOutStream : public std::basic_ostream<T>
{
	typedef std::basic_ostream<T> base;

	BufferType m_buf;

public:
	GenericOutStream(std::ios_base::fmtflags f = std::ios_base::fmtflags()) : base(&m_buf)
	{
		base::setf(f);
	}

	const BufferType & rdbuf()
	{
		return m_buf;
	}
};

