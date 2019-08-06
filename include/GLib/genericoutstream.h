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
		GenericOutStream(std::ios_base::fmtflags f = std::ios_base::fmtflags()) : Base(&buffer)
		{
			Base::setf(f);
		}

		const BufferType & Buffer() const
		{
			return buffer;
		}
	};
}