#pragma once

#include <streambuf>
#include <vector>

class Buffer : public std::basic_streambuf<char>
{
	static constexpr int bufferSize = 256;
	typedef std::vector<char> bufferType;
	typedef basic_streambuf<char> base;
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

