#pragma once

#include <utility>

namespace GLib::Win
{
	template <typename Wrapped, typename RawType>
	class Transfer
	{
		Wrapped & wrapped;
		RawType rawValue;

	public:
		explicit Transfer(Wrapped & wrapped)
			: wrapped(wrapped)
			, rawValue()
		{}

		Transfer(const Transfer & ref) = delete;
		Transfer(Transfer && ref) = delete;
		Transfer & operator=(const Transfer &) = delete;
		Transfer & operator=(Transfer &&) = delete;

		~Transfer()
		{
			Wrapped newValue = Wrapped::Attach(rawValue);
			std::swap(wrapped, newValue);
		}

		RawType * Raw()
		{
			return &rawValue;
		}

		void ** Void()
		{
			return reinterpret_cast<void **>(&rawValue); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) required for COM interop
		}
	};
}