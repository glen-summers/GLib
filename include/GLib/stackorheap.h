#ifndef STACK_OR_HEAP_H
#define STACK_OR_HEAP_H

namespace GLib
{
	namespace Util
	{
		// ?rework as MakeStackOrHeap<size>(actual) -> actual < size ? Stack() : Heap()

		template <typename T, size_t StackElementCount>
		class StackOrHeap
		{
				struct Heap { T* p; size_t elementCount; };
				bool heapInUse = false;
				union
				{
						T stack[StackElementCount];
						Heap heap;
				};

				size_t GetSize() const { return heapInUse ? heap.elementCount : StackElementCount; }
				const T* GetPtr() const { return heapInUse ? heap.p : stack; }

		public:
				StackOrHeap() = default;
				StackOrHeap(const StackOrHeap &) = delete;
				StackOrHeap & operator=(const StackOrHeap &) = delete;
				StackOrHeap(StackOrHeap &&) = delete;
				StackOrHeap & operator=(StackOrHeap &&) = delete;

				~StackOrHeap()
				{
						if (heapInUse)
						{
								delete[] heap.p;
						}
				}

				void EnsureSize(size_t newElementCount)
				{
						if ((newElementCount > StackElementCount && !heapInUse) || (heapInUse && newElementCount > heap.elementCount))
						{
								if (heapInUse)
								{
										delete[] heap.p;
								}
								heap.p = new T[newElementCount];
								heap.elementCount = newElementCount;
								heapInUse = true;
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
		};
	}
}
#endif // STACK_OR_HEAP_H
