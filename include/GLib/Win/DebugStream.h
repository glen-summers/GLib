#pragma once

#include <Windows.h>

#include "GLib/genericoutstream.h"
#include "GLib/vectorstreambuffer.h"
#include "GLib/cvt.h"

namespace GLib::Win::Debug
{
	namespace Detail
	{
		static constexpr auto DefaultCapacity = 256;
		using Buffer = GLib::Util::VectorStreamBuffer<char, DefaultCapacity>;

		class DebugBuffer : public Buffer
		{
			int_type overflow(int_type c) override
			{
				c = Buffer::overflow(c);
				if (c == traits_type::to_char_type('\n'))
				{
					Write(Get());
				}
				return c;
			}

			void Write(const char* s)
			{
				::OutputDebugStringW(Cvt::a2w(s).c_str());
				Reset();
			}
		};
	}

	inline std::ostream & Stream()
	{
		static thread_local GLib::Util::GenericOutStream<char, Detail::DebugBuffer> debugStream(std::ios_base::boolalpha);
		return debugStream;
	}
}