#pragma once

#include <GLib/Win/ErrorCheck.h>

namespace GLib::Win
{
	struct PaintInfo;

	namespace Detail
	{
		struct PaintEnder
		{
			void operator()(const PaintInfo * info) const;
		};

		using PaintHolder = std::unique_ptr<PaintInfo, PaintEnder>;
	}

	struct PaintInfo
	{
		PAINTSTRUCT PaintStruct;
		HWND Window;
		HDC DeviceContext;
	};

	inline void Detail::PaintEnder::operator()(const PaintInfo * info) const
	{
		Util::WarnAssertTrue(EndPaint(info->Window, &info->PaintStruct), "EndPaint");
	}

	class Painter
	{
		PaintInfo info;
		Detail::PaintHolder p;

	public:
		Painter(const PaintInfo & info)
			: info {info}
			, p {&this->info, Detail::PaintEnder {}}
		{}
	};
}