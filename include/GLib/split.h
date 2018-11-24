#pragma once

#include <string>

namespace GLib
{
	namespace Util
	{
		class Splitter
		{
			const std::string value;
			const std::string delimiter;

		public:
			Splitter(const std::string & value, const std::string & delimiter = ",")
				: value(value)
				, delimiter(delimiter)
			{}

			class iterator : std::iterator<std::input_iterator_tag, std::string>
			{
				const Splitter * const splitter;
				std::size_t current, previous;

			public:
				iterator(const Splitter & splitter)
					: splitter(&splitter)
					, current(CheckEnd(splitter.value.find(splitter.delimiter, 0)))
					, previous()
				{}

				iterator() : splitter(), current(std::string::npos), previous(std::string::npos)
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
					if (current == splitter->value.size())
					{
						current = std::string::npos;
					}
					else if (current != std::string::npos)
					{
						previous = current + splitter->delimiter.size();
						current = CheckEnd(splitter->value.find(splitter->delimiter, previous));
					}

					return *this;
				}

				std::string operator*() const
				{
					auto n = current != std::string::npos
						? current - previous
						: std::string::npos;
					return splitter->value.substr(previous, n);
				}

			private:
				std::size_t CheckEnd(std::size_t value)
				{
					return !splitter->value.empty() && value == std::string::npos ? splitter->value.size() : value;
				}
			};

			iterator begin() const
			{
				return iterator(*this);
			}

			static iterator end()
			{
				return iterator();
			}
		};
	}
}
