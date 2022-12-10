#pragma once

#include <Windows.h>

#include <GLib/Cvt.h>
#include <GLib/GenericOutStream.h>
#include <GLib/VectorStreamBuffer.h>

namespace GLib::Win::Debug
{
	namespace Detail
	{
		static constexpr auto DefaultCapacity = 256;
		using Buffer = GLib::Util::VectorStreamBuffer<char, DefaultCapacity>;

		class DebugBuffer : public Buffer
		{
			int_type overflow(int_type const chr) override
			{
				if (Buffer::overflow(chr) == traits_type::to_char_type('\n'))
				{
					Write(Get());
				}
				return chr;
			}

			void Write(std::string_view const value)
			{
				OutputDebugStringW(Cvt::A2W(value).c_str());
				Reset();
			}
		};
	}

	inline std::ostream & Stream()
	{
		static thread_local GLib::Util::GenericOutStream<char, Detail::DebugBuffer> debugStream(std::ios_base::boolalpha);
		return debugStream.Stream();
	}
}