#pragma once

#include <iterator>
#include <stdexcept>
#include <string>

namespace GLib::Util
{
	namespace Detail
	{
		template <typename CharType> const CharType * DeFaultDeliminator();
		template <> inline const char * DeFaultDeliminator() { return ","; }
		template <> inline const wchar_t * DeFaultDeliminator() { return L","; }

		template <typename StringType>
		class Splitter
		{
			using T = typename StringType::value_type;
			StringType value;
			StringType delimiter;

		public:
			Splitter(StringType value, const StringType & delimiter = DeFaultDeliminator<typename StringType::value_type>())
				: value(move(value))
				, delimiter(delimiter)
			{
				if (delimiter.empty())
				{
					throw std::logic_error("Delimiter is empty");
				}
			}

			class iterator
			{
				const Splitter * const splitter;
				std::size_t current, nextDelimiter;

			public:
				using iterator_category = std::forward_iterator_tag;
				using value_type = StringType;
				using difference_type = void;
				using pointer = void;
				using reference = void;

				iterator(const Splitter & splitter)
					: splitter(&splitter)
					, current(0)
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

	using Splitter = Detail::Splitter<std::string>;
	using SplitterView = Detail::Splitter<std::string_view>;

	template <typename StringType, typename OutputIterator>
	void Split(const StringType & value, OutputIterator it, const StringType & delimiter = Detail::DeFaultDeliminator<typename StringType::value_type>())
	{
		Detail::Splitter<StringType> splitter { value, delimiter };
		std::copy(splitter.begin(), splitter.end(), it);
	}

	template <typename Predicate, typename OutYes, typename OutNo>
	inline void Split(const std::string_view & value, Predicate predicate, OutYes outYes, OutNo outNo)
	{
		for(auto it = value.begin(); it != value.end();)
		{
			auto falseStart = std::find_if_not(it, value.end(), predicate);
			if (falseStart != it)
			{
				outYes(std::string_view {&*it, static_cast<size_t>(falseStart - it)});
			}

			auto trueStart = std::find_if(falseStart, value.end(), predicate);
			if (trueStart != falseStart)
			{
				outNo(std::string_view {&*falseStart, static_cast<size_t>(trueStart - falseStart)});
			}
			it = trueStart;
		}
	}
}