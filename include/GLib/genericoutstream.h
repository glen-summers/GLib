#pragma once

#include <ostream>

namespace GLib::Util
{
	template <typename T, class BufferType>
	class GenericOutStream : public std::basic_ostream<T>
	{
		using Base = std::basic_ostream<T>;

		BufferType buffer;

	public:
		explicit GenericOutStream(std::ios_base::fmtflags flags = {}) : Base(&buffer)
		{
			Base::setf(flags);
		}

		BufferType & Buffer()
		{
			return buffer;
		}
	};
}