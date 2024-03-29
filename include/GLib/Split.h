#pragma once

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>

namespace GLib::Util
{
	namespace Detail
	{
		template <typename CharType>
		CharType const * DefaultDelimiter();

		template <>
		inline char const * DefaultDelimiter()
		{
			return ",";
		}

		template <>
		inline wchar_t const * DefaultDelimiter()
		{
			return L",";
		}

		template <typename StringType>
		class Splitter
		{
			using T = typename StringType::value_type;
			StringType value;
			StringType delimiter;

		public:
			explicit Splitter(StringType value, StringType const & delimiter = DefaultDelimiter<typename StringType::value_type>())
				: value(std::move(value))
				, delimiter(delimiter)
			{
				if (delimiter.empty())
				{
					throw std::logic_error("Delimiter is empty");
				}
			}

			class Iterator
			{
				Splitter const * const splitter;
				size_t current, nextDelimiter;

			public:
				// ReSharper disable All
				using iterator_category = std::forward_iterator_tag;
				using value_type = StringType;
				using difference_type = void;
				using pointer = void;
				using reference = void;

				// ReSharper restore All

				explicit Iterator(Splitter const & splitter)
					: splitter(&splitter)
					, current(0)
					, nextDelimiter(splitter.value.find(splitter.delimiter, 0))
				{}

				Iterator()
					: splitter()
					, current(StringType::npos)
					, nextDelimiter(StringType::npos)
				{}

				bool operator==(Iterator const & iter) const
				{
					return current == iter.current;
				}

				bool operator!=(Iterator const & iter) const
				{
					return !(*this == iter);
				}

				Iterator operator++()
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
					auto end = nextDelimiter != StringType::npos ? nextDelimiter : splitter->value.size();
					return splitter->value.substr(current, end - current);
				}
			};

			[[nodiscard]] Iterator begin() const
			{
				return Iterator(*this);
			}

			[[nodiscard]] Iterator end() const
			{
				return Iterator();
			}
		};
	}

	using Splitter = Detail::Splitter<std::string>;
	using SplitterView = Detail::Splitter<std::string_view>;

	template <typename StringType, typename OutputIterator>
	void Split(StringType const & value, OutputIterator iter,
						 StringType const & delimiter = Detail::DefaultDelimiter<typename StringType::value_type>())
	{
		Detail::Splitter<StringType> const splitter {value, delimiter};
		std::copy(splitter.begin(), splitter.end(), iter);
	}

	template <typename Predicate, typename OutYes, typename OutNo>
	void Split(std::string_view value, Predicate predicate, OutYes outYes, OutNo outNo)
	{
		for (auto it = value.begin(); it != value.end();)
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