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
				if (Base::traits_type::eq_int_type(c, Base::traits_type::eof()))
				{
					return Base::traits_type::not_eof(c);
				}
				buffer.push_back(Base::traits_type::to_char_type(c));
				return c;
			}
		};
	}
}