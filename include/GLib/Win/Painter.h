#pragma once

#include <GLib/Win/ErrorCheck.h>

namespace GLib::Win
{
	struct PaintInfo;

	namespace Detail
	{
		struct PaintEnder
		{
			using pointer = const PaintInfo *;
			void operator()(const PaintInfo * info) const;
		};

		using PaintHolder = std::unique_ptr<PaintInfo, PaintEnder>;
	}

	struct PaintInfo
	{
		PAINTSTRUCT ps;
		HWND wnd;
		HDC dc;
	};

	inline void Detail::PaintEnder::operator()(const PaintInfo * info) const
	{
		Util::WarnAssertTrue(::EndPaint(info->wnd, &info->ps), "EndPaint");
	}

	class Painter
	{
		PaintInfo info;
		Detail::PaintHolder p;

	public:
		Painter(const PaintInfo & info)
			: info{info}
			, p{&this->info, Detail::PaintEnder{}}
		{}
	};
}