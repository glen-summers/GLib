#pragma once

#include <GLib/Win/ErrorCheck.h>

namespace GLib::Win
{
	struct PaintInfo;

	namespace Detail
	{
		struct PaintEnder
		{
			void operator()(PaintInfo const * info) const;
		};

		using PaintHolder = std::unique_ptr<PaintInfo, PaintEnder>;
	}

	struct PaintInfo
	{
		PAINTSTRUCT PaintStruct;
		HWND Window;
		HDC DeviceContext;
	};

	inline void Detail::PaintEnder::operator()(PaintInfo const * info) const
	{
		Util::WarnAssertTrue(EndPaint(info->Window, &info->PaintStruct), "EndPaint");
	}

	class Painter
	{
		PaintInfo info;
		Detail::PaintHolder p;

	public:
		explicit Painter(PaintInfo const & info)
			: info {info}
			, p {&this->info, Detail::PaintEnder {}}
		{}
	};
}