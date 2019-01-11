#ifndef STACK_OR_HEAP_H
#define STACK_OR_HEAP_H

#include <memory>

namespace GLib
{
	namespace Util
	{
		// ?rework as MakeStackOrHeap<size>(actual) -> actual < size ? Stack() : Heap()

		template <typename T, size_t StackElementCount>
		class StackOrHeap
		{
			size_t heapSize;
			union
			{
				T stack[StackElementCount];
				std::unique_ptr<T[]> heap;
			};

		public:
			StackOrHeap() : heapSize{} {}
			StackOrHeap(const StackOrHeap &) = delete;
			StackOrHeap & operator=(const StackOrHeap &) = delete;
			StackOrHeap(StackOrHeap &&) = delete;
			StackOrHeap & operator=(StackOrHeap &&) = delete;

			~StackOrHeap()
			{
				DeleteHeap();
			}

			void EnsureSize(size_t newElementCount)
			{
				if ((newElementCount > StackElementCount && !HeapInUse()) || (HeapInUse() && newElementCount > heapSize))
				{
					AllocateHeap(newElementCount);
				}
				// could go back to stack if (heapInUse && newElementCount <= StackElementCount)
			}

			const T* Get() const { return GetPtr(); }
			T* Get() { return const_cast<T*>(GetPtr()); }

			const T* operator->() const { return Get(); }
			T* operator->() { return Get(); }

			size_t size() const { return GetSize(); }

			const T * begin() const { return Get(); }
			T * begin() { return Get(); }

			const T * end() const { return Get() + size(); }
			T * end() { return Get() + size(); }

		private:
			bool HeapInUse() const { return heapSize != 0; }
			size_t GetSize() const { return HeapInUse() ? heapSize : StackElementCount; }
			const T* GetPtr() const { return HeapInUse() ? heap.get() : stack; }

			void DeleteHeap()
			{
				if (HeapInUse())
				{
					heap.std::template unique_ptr<T[]>::~unique_ptr();
				}
			}

			void AllocateHeap(size_t size)
			{
				DeleteHeap();
				new (&heap) std::unique_ptr<T[]>(std::make_unique<T[]>(size));
				heapSize = size;
			}
		};
	}
}
#endif // STACK_OR_HEAP_H
