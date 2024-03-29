#pragma once

#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/Transfer.h>

namespace GLib::Win
{
	namespace LocalDetail
	{
		struct LocalFreer
		{
			void operator()(HLOCAL const handle) const noexcept
			{
#pragma warning(push)
#pragma warning(disable : 6001)
				Util::WarnAssertTrue(LocalFree(handle) == nullptr, "LocalFree");
#pragma warning(pop)
			}
		};
	}

	template <typename T>
	class Local
	{
		using Ptr = std::unique_ptr<T, LocalDetail::LocalFreer>;
		Ptr p;

	public:
		explicit Local(T * const value = nullptr)
			: p(value)
		{}

		[[nodiscard]] T const * Get() const
		{
			return p.get();
		}

		static Local Attach(T * value)
		{
			return Local {value};
		}
	};

	template <typename T>
	auto GetAddress(Local<T> & value) noexcept
	{
		return Transfer<Local<T>, T *>(value);
	}
}
