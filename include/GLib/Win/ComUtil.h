#pragma once

#include "GLib/Win/ComPtr.h"
#include "GLib/Win/ComErrorCheck.h"

namespace GLib
{
	namespace Win
	{
		template <typename Target, typename Source>
		ComPtr<Target> ComCast(const Source & source)
		{
			ComPtr<Target> value;
			if (source)
			{
				CheckHr(source->QueryInterface(&value), "QueryInterface");
			}
			return value;
		}
	}
}