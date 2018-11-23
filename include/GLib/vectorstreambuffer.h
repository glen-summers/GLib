#pragma once

#include <streambuf>
#include <vector>

namespace GLib
{
	namespace Util
	{
		template <typename T = char, size_t InitialCapacity = 256>
		class VectorStreamBuffer : public std::basic_streambuf<T>
		{
			typedef std::basic_streambuf<T> Base;
			typedef std::vector<T> BufferType;
			mutable BufferType buffer;

		public:
			using typename Base::int_type;
			using typename Base::traits_type;

			VectorStreamBuffer()
			{
				buffer.reserve(InitialCapacity);
			}

			const T * Get() const
			{
				if (buffer.empty() || *buffer.rbegin() != T())
				{
					buffer.push_back(T());
				}
				return buffer.data();
			}

			void Reset() const
			{
				// decay size if large and not often used
				// poss use list of chunks that can be shared?
				buffer.clear();
				buffer.reserve(InitialCapacity);
			}

		protected:
			int_type overflow(int_type c) override
			{
				if (traits_type::eq_int_type(c, traits_type::eof()))
				{
					return traits_type::not_eof(c);
				}
				buffer.push_back(traits_type::to_char_type(c));
				return c;
			}
		};
	}
}