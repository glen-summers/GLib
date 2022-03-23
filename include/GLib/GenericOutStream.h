#pragma once

#include <ostream>

namespace GLib::Util
{
	template <typename T, typename BufferType>
	class GenericOutStream
	{
		std::basic_ostream<T> stream;
		BufferType buffer;

	public:
		explicit GenericOutStream(std::ios_base::fmtflags flags = {})
			: stream(&buffer)
		{
			stream.setf(flags);
		}

		std::basic_ostream<T> & Stream()
		{
			return stream;
		}

		BufferType & Buffer()
		{
			return buffer;
		}
	};
}