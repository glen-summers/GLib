#pragma once

#include <streambuf>
#include <vector>

namespace GLib::Util
{
	// convert to circular buffer
	template <typename T, size_t DefaultCapacity>
	class VectorStreamBuffer : public std::basic_streambuf<T>
	{
		using Base = std::basic_streambuf<T>;
		using BufferType = std::vector<T>;
		BufferType buffer;

	public:
		using typename Base::int_type;

		explicit VectorStreamBuffer(size_t initialCapacity = DefaultCapacity)
		{
			buffer.reserve(initialCapacity);
		}

		std::basic_string_view<T> Get()
		{
			return {buffer.data(), buffer.size()};
		}

		void Reset()
		{
			// decay size if large and not often used
			// poss use list of chunks that can be shared?
			buffer.clear();
			buffer.reserve(DefaultCapacity);
		}

	protected:
		int_type overflow(int_type c) override
		{
			if (Base::traits_type::eq_int_type(c, Base::traits_type::eof()))
			{
				return Base::traits_type::not_eof(c);
			}
			buffer.push_back(Base::traits_type::to_char_type(c));
			return c;
		}
	};
}