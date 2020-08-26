#pragma once

#include <GLib/Win/ComErrorCheck.h>

namespace GLib::Win
{
	namespace Detail
	{
		struct ComUninitialiser
		{
			void operator()(void * unused) const noexcept
			{
				(void) unused;
				::CoUninitialize();
			}
		};

		enum class Apartment : unsigned long
		{
			Multithreaded = COINITBASE_MULTITHREADED,
			Singlethreaded = COINIT_APARTMENTTHREADED
		};
	}

	template <Detail::Apartment appartment>
	class ComInitialiser
	{
		std::unique_ptr<void, Detail::ComUninitialiser> com;

	public:
		ComInitialiser()
			: com {this}
		{
			GLib::Win::CheckHr(::CoInitializeEx(nullptr, static_cast<DWORD>(appartment)), "CoInitializeEx");
		}
	};

	using Mta = ComInitialiser<Detail::Apartment::Multithreaded>;
	using Sta = ComInitialiser<Detail::Apartment::Singlethreaded>;
}