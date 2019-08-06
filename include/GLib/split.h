#pragma once

#include <string>
#include <algorithm>

namespace GLib::Util
{
	namespace Detail
	{
		template <typename T>
		class Splitter
		{
			using StringType = std::basic_string<T>;
			const StringType value;
			const StringType delimiter;

		public:
			Splitter(const StringType & value, const StringType & delimiter = std::basic_string<T>(1, T(',')))
				: value(value)
				, delimiter(delimiter)
			{}

			class iterator
			{
				const Splitter * const splitter;
				std::size_t current, nextDelimiter;

			public:
				using iterator_category = std::forward_iterator_tag;
				using value_type = void; // StringType;
				using difference_type = void;
				using pointer = void;
				using reference = void;

				iterator(const Splitter & splitter)
					: splitter(&splitter)
					, current(splitter.value.empty() ? StringType::npos : 0)
					, nextDelimiter(splitter.value.find(splitter.delimiter, 0))
				{}

				iterator() : splitter(), current(StringType::npos), nextDelimiter(StringType::npos)
				{}

				bool operator==(const iterator& it) const
				{
					return current == it.current;
				}

				bool operator!=(const iterator& it) const
				{
					return !(*this == it);
				}

				iterator operator++()
				{
					if (nextDelimiter != StringType::npos)
					{
						current = nextDelimiter + splitter->delimiter.size();
						nextDelimiter = splitter->value.find(splitter->delimiter, current);
					}
					else
					{
						current = nextDelimiter;
					}

					return *this;
				}

				StringType operator*() const
				{
					auto end = nextDelimiter != StringType::npos
						? nextDelimiter
						: splitter->value.size();
					return splitter->value.substr(current, end - current);
				}
			};

			iterator begin() const
			{
				return iterator(*this);
			}

			iterator end() const
			{
				return iterator();
			}
		};
	}

	using Splitter = Detail::Splitter<char>;

	template <typename T, typename OutputIterator>
	void Split(const std::basic_string<T> & value, OutputIterator it,
		const std::basic_string<T> & delimiter = std::basic_string<T>(1, T(',')))
	{
		Detail::Splitter<T> splitter { value, delimiter };
		std::copy(splitter.begin(), splitter.end(), it);
	}
}