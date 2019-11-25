#pragma once

#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

template <typename T> struct Reverse { T & iterable; };
template <typename T> auto begin(const Reverse<T> & w) { return std::crbegin(w.iterable); }
template <typename T> auto end(const Reverse<T> & w) { return std::crend(w.iterable); }
template <typename T> Reverse<T> Reverser(T && iterable) { return { iterable }; }

inline bool Parse(std::string & name, unsigned int imbalance, char open, char close)
{
	std::ostringstream s;
	unsigned int depth{};

	for (auto c : name)
	{
		if (c == open)
		{
			if (++depth == 1)
			{
				s.put(c);
				s.put('T');
			}
		}
		else if (c == close)
		{
			if (depth != 0 && --depth == 0)
			{
				s.put(c);
			}
			else if (depth == 0 && imbalance != 0)
			{
				--imbalance;
				s.put(c);
			}
			else if (depth == 0)
			{
				return false;
			}
		}
		else
		{
			if (depth == 0)
			{
				s.put(c);
			}
		}
	}
	name = s.str();
	return true;
}

inline void RemoveTemplateDefinitions(std::string & name)
{
	int left{};
	int right{};

	for (auto c : name)
	{
		switch (c)
		{
			case '<':
			{
				++left;
				break;
			}

			case '>':
			{
				++right;
				break;
			}

			default:;
		}
	}

	if (left == 0 && right == 0)
	{
		return;
	}

	bool ok{};
	if (left < right)
	{
		ok = Parse(name, right - left, '<', '>');
	}
	else
	{
		std::reverse(name.begin(), name.end());
		ok = Parse(name, right - left, '>', '<');
		std::reverse(name.begin(), name.end());
	}

	if (!ok)
	{
		throw std::runtime_error("Unable to parse symbol: " + name);
	}
}
