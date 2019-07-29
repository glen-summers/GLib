#pragma once

#include "GLib/Win/WinException.h"

namespace GLib
{
	namespace Win
	{
		class ComException : public WinException
		{
		public:
			ComException(std::string message, HRESULT hr)
				: WinException(move(message), hr)
			{}
		};
	}
}
