#pragma once

#include <GLib/Win/WinException.h>

namespace GLib::Win
{
	class ComException : public WinException
	{
	public:
		ComException(std::string const & message, HRESULT const result)
			: WinException(message, result)
		{}
	};
}