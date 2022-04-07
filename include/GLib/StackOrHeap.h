#pragma once

#include <array>
#include <variant>
#include <vector>

namespace GLib::Util
{
	// ?rework as MakeStackOrHeap<size>(actual) -> actual < size ? Stack() : Heap()
	template <typename T, size_t StackElementCount>
	class StackOrHeap
	{
		using Stack = std::array<T, StackElementCount>;
		using Heap = std::vector<T>;

		size_t heapSize {};
		std::variant<Stack, Heap> storage;

	public:
		StackOrHeap() = default;
		StackOrHeap(const StackOrHeap &) = delete;
		StackOrHeap(StackOrHeap &&) = delete;
		StackOrHeap & operator=(const StackOrHeap &) = delete;
		StackOrHeap & operator=(StackOrHeap &&) = delete;
		~StackOrHeap() = default;

		void EnsureSize(size_t newElementCount)
		{
			if ((newElementCount > StackElementCount && !HeapInUse()) || (HeapInUse() && newElementCount > heapSize))
			{
				AllocateHeap(newElementCount);
			}
			// could go back to stack if (heapInUse && newElementCount <= StackElementCount)
		}

		[[nodiscard]] size_t Size() const
		{
			return GetSize();
		}

		T * Get()
		{
			return HeapInUse() ? &std::get<1>(storage)[0] : std::get<0>(storage).data();
		}

		[[nodiscard]] const T * Get() const
		{
			return HeapInUse() ? &std::get<1>(storage)[0] : std::get<0>(storage).data();
		}

	private:
		[[nodiscard]] bool HeapInUse() const
		{
			return storage.index() != 0;
		}

		[[nodiscard]] size_t GetSize() const
		{
			return HeapInUse() ? heapSize : StackElementCount;
		}

		void AllocateHeap(size_t size)
		{
			storage = std::vector<T>(size);
			heapSize = size;
		}
	};

	constexpr auto DefaultStackReserveSize = 256;
	using CharBuffer = StackOrHeap<char, DefaultStackReserveSize>;
	using WideCharBuffer = StackOrHeap<wchar_t, DefaultStackReserveSize>;
}
