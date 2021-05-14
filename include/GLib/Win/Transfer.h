#pragma once

#include <utility>

namespace GLib::Win
{
	template <typename Wrapped, typename Raw>
	class Transfer
	{
		Wrapped & wrapped;
		Raw rawValue;

	public:
		explicit Transfer(Wrapped & wrapped)
			: wrapped(wrapped)
			, rawValue()
		{}

		Transfer(const Transfer & ref) = delete;
		Transfer(Transfer && ref) = delete;
		const Transfer & operator=(const Transfer &) = delete;
		const Transfer & operator=(Transfer &&) = delete;

		~Transfer()
		{
			Wrapped newValue = Wrapped::Attach(rawValue);
			std::swap(wrapped, newValue);
		}

		operator Raw *()
		{
			return &rawValue;
		}

		operator void **()
		{
			return reinterpret_cast<void **>(&rawValue); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) for IUnknown factory methods
		}
	};
}