#pragma once

#include <GLib/Win/ComErrorCheck.h>

namespace GLib::Win
{
	namespace Detail
	{
		struct ComUnInitialise
		{
			void operator()(void * unused) const noexcept
			{
				static_cast<void>(unused);
				CoUninitialize();
			}
		};

		enum class Apartment : unsigned long
		{
			MultiThreaded = COINITBASE_MULTITHREADED,
			SingleThreaded = COINIT_APARTMENTTHREADED
		};
	}

	template <Detail::Apartment Apartment>
	class ComInitialise
	{
		std::unique_ptr<void, Detail::ComUnInitialise> com;

	public:
		ComInitialise()
			: com {this}
		{
			CheckHr(CoInitializeEx(nullptr, static_cast<DWORD>(Apartment)), "CoInitializeEx");
		}
	};

	using Mta = ComInitialise<Detail::Apartment::MultiThreaded>;
	using Sta = ComInitialise<Detail::Apartment::SingleThreaded>;
}