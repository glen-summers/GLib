#pragma once

#include <sstream>
#include <stdexcept>
#include <string>

inline bool Parse(std::string & name, unsigned int imbalance, char const open, char const close)
{
	std::ostringstream stm;
	unsigned int depth {};

	for (auto const chr : name)
	{
		if (chr == open)
		{
			if (++depth == 1)
			{
				stm.put(chr);
				stm.put('T');
			}
		}
		else if (chr == close)
		{
			if (depth != 0 && --depth == 0)
			{
				stm.put(chr);
			}
			else if (depth == 0 && imbalance != 0)
			{
				--imbalance;
				stm.put(chr);
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
				stm.put(chr);
			}
		}
	}
	name = stm.str();
	return true;
}

inline void RemoveTemplateDefinitions(std::string & name)
{
	int left {};
	int right {};

	for (auto const chr : name)
	{
		switch (chr)
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

	bool okay {};
	if (left < right)
	{
		okay = Parse(name, right - left, '<', '>');
	}
	else
	{
		std::ranges::reverse(name);
		okay = Parse(name, right - left, '>', '<');
		std::ranges::reverse(name);
	}

	if (!okay)
	{
		throw std::runtime_error("Unable to parse symbol: " + name);
	}
}
