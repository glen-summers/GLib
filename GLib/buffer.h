#pragma once

#include <streambuf>
#include <vector>

template <typename T=char, size_t bufferSize = 256>
class Buffer : public std::basic_streambuf<T>
{
	typedef std::vector<T> bufferType;
	typedef std::basic_streambuf<T> base;
	using typename base::int_type;
	using typename base::traits_type;

	mutable bufferType buffer;

	int_type overflow(int_type c) override
	{
		if (traits_type::eq_int_type(c, traits_type::eof()))
		{
			return traits_type::not_eof(c);
		}
		buffer.push_back(traits_type::to_char_type(c));
		return c;
	}

public:
	Buffer()
	{
		buffer.reserve(bufferSize);
	}

	const char * Get() const
	{
		buffer.push_back(0);
		return buffer.data();
	}

	void Reset() const
	{
		// decay size if large and not often used
		// poss use list of chunks that can be shared?
		buffer.clear();
	}
};

