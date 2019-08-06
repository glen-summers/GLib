#ifndef STACK_OR_HEAP_H
#define STACK_OR_HEAP_H

#include <memory>
#include <variant>
#include <array>

namespace GLib::Util
{
	// ?rework as MakeStackOrHeap<size>(actual) -> actual < size ? Stack() : Heap()

	template <typename T, size_t StackElementCount>
	class StackOrHeap
	{
		size_t heapSize;
		std::variant<std::array<T, StackElementCount>, std::unique_ptr<T[]>> storage; // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)

	public:
		StackOrHeap() : heapSize{}
		{}

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

		T* Get()
		{
			return HeapInUse() ? std::get<1>(storage).get() : std::get<0>(storage).data();
		}

		const T* Get() const
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
			storage = std::make_unique<T[]>(size);
			heapSize = size;
		}
	};
}
#endif // STACK_OR_HEAP_H
