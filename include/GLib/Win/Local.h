#pragma once

#include <GLib/Win/ErrorCheck.h>
#include <GLib/Win/Transfer.h>

#include <cassert>

namespace GLib::Win
{
	namespace LocalDetail
	{
		struct LocalFreer
		{
			void operator()(HLOCAL p) const noexcept
			{
				Util::WarnAssertTrue(::LocalFree(p) == nullptr, "LocalFree");
			}
		};
	}

	template <typename T>
	class Local
	{
		using Ptr = std::unique_ptr<T, LocalDetail::LocalFreer>;
		Ptr p;

	public:
		explicit Local(T * value = nullptr)
			: p(value)
		{}

		const T * Get() const
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
