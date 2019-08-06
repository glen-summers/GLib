#pragma once

#include "GLib/Win/WinException.h"

namespace GLib::Win
{
	class ComException : public WinException
	{
	public:
		ComException(std::string message, HRESULT hr)
			: WinException(move(message), hr)
		{}
	};
}