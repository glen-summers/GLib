#pragma once

#include <GLib/Win/WinException.h>

namespace GLib::Win
{
	class ComException : public WinException
	{
	public:
		ComException(const std::string & message, HRESULT hr)
			: WinException(message, hr)
		{}
	};
}