#ifndef STACK_OR_HEAP_H
#define STACK_OR_HEAP_H

#include <array>
#include <memory>
#include <variant>

namespace GLib::Util
{
	// ?rework as MakeStackOrHeap<size>(actual) -> actual < size ? Stack() : Heap()
	template <typename T, size_t StackElementCount>
	class StackOrHeap
	{
		using Stack = std::array<T, StackElementCount>;
		using Heap = std::unique_ptr<T[]>; // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)

		size_t heapSize {};
		std::variant<Stack, Heap> storage;

	public:
		StackOrHeap() = default;
		StackOrHeap(const StackOrHeap &) = delete;
		StackOrHeap & operator=(const StackOrHeap &) = delete;
		StackOrHeap(StackOrHeap &&) = delete;
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

		size_t size() const
		{
			return GetSize();
		}

		T * Get()
		{
			return HeapInUse() ? std::get<1>(storage).get() : std::get<0>(storage).data();
		}

		const T * Get() const
		{
			return HeapInUse() ? std::get<1>(storage).get() : std::get<0>(storage).data();
		}

	private:
		bool HeapInUse() const
		{
			return storage.index() != 0;
		}

		size_t GetSize() const
		{
			return HeapInUse() ? heapSize : StackElementCount;
		}

		void AllocateHeap(size_t size)
		{
			storage = std::make_unique<T[]>(size); // NOLINT(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
			heapSize = size;
		}
	};

	constexpr auto DefaultStackReserveSize = 256;
	using CharBuffer = Util::StackOrHeap<char, DefaultStackReserveSize>;
	using WideCharBuffer = Util::StackOrHeap<wchar_t, DefaultStackReserveSize>;
}
#endif // STACK_OR_HEAP_H
