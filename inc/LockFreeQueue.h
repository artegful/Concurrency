
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <cassert>

#include "CountedPtr.h"

namespace LockFree
{
    struct Counter
    {
        int internalCounter : 29;
        int externalCounters : 3;
    };

    template<typename T>
    struct Node
    {
        std::atomic<T*> data;
        std::atomic<Counter> counter;
        std::atomic<CountedPtr<Node<T>>> next;

        Node() : 
            data(nullptr), counter(Counter{ 0, 2 }), next(CountedPtr<Node<T>>{ nullptr, 0 }) 
        {}

        void Release()
        {
            Counter oldCounter = counter.load(std::memory_order_relaxed);
            Counter newCounter;

            do
            {
                assert(oldCounter.internalCounter != 0 || oldCounter.externalCounters != 0);
                newCounter = oldCounter;
                --newCounter.internalCounter;
            } 
            while (!counter.compare_exchange_weak(oldCounter, newCounter, 
                std::memory_order_release, std::memory_order_relaxed));

            if (newCounter.internalCounter == 0 && newCounter.externalCounters == 0)
            {
                delete this;
            }
        }
    };

    template<typename T>
    class Queue
    {
    public:
        using CountedPtrElement = CountedPtr<Node<T>>;

        Queue()
        {
            CountedPtrElement blankElement { new Node<T>(), 1 };

            head.store(blankElement, std::memory_order_release);
            tail.store(blankElement, std::memory_order_release);
        }

        ~Queue()
        {
            while (Pop());

            Node<T>* blankNodePtr = head.load(std::memory_order_relaxed).ptr;
            delete blankNodePtr;
        }

        void Push(const T& value)
        {
            std::unique_ptr<T> data = std::make_unique<T>(value);

            CountedPtrElement newElement { new Node<T>(), 1 };

            for (;;)
            {
                CountedPtrElement oldTail = tail.load();
                IncreaseRefCount(oldTail, tail);

                CountedPtrElement oldNext = { nullptr, 0 };
                T* oldData = nullptr;
                
                if (oldTail->data.compare_exchange_strong(oldData, data.get(), 
                    std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    if (!oldTail->next.compare_exchange_strong(oldNext, newElement,
                        std::memory_order_acq_rel, std::memory_order_acquire))
                    {
                        delete newElement.ptr;
                        newElement = oldNext;
                    }

                    UpdateTail(oldTail, newElement);
                    data.release();
                    break;
                }
                else
                {
                    if (oldTail->next.compare_exchange_strong(oldNext, newElement,
                        std::memory_order_acq_rel, std::memory_order_acquire))
                    {
                        oldNext = newElement;
                        newElement.ptr = new Node<T>();
                    }

                    UpdateTail(oldTail, oldNext);
                }
            }
        }

        std::unique_ptr<T> Pop()
        {
            CountedPtrElement oldHead = head.load(std::memory_order_relaxed);

            for (;;)
            {
                IncreaseRefCount(oldHead, head);

                if (oldHead.ptr == tail.load(std::memory_order_relaxed).ptr)
                {
                    oldHead->Release();
                    return {};
                }

                auto next = oldHead->next.load(std::memory_order_relaxed);
                CountedPtrElement oldHeadCopy = oldHead;
                if (head.compare_exchange_strong(oldHeadCopy, next,
                    std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    // Push relies on data being nullptr on the last element
                    // Setting it to nullptr here leads to leaking elements.
                    // Tail is moved forward only after setting data on the
                    // old tail, so if head.ptr != tail.ptr, then data changes
                    // visibility is guaranteed here, but assert to be sure anyway  
                    T* dataPtr = oldHead->data.load(std::memory_order_relaxed);
                    assert(dataPtr);
                    ReleaseCounter(oldHead);

                    return std::unique_ptr<T> { dataPtr };
                }

                oldHead->Release();
            }
        }

    private:
        static void IncreaseRefCount(CountedPtrElement& old, std::atomic<CountedPtrElement>& atomic)
        {
            CountedPtrElement newCountedPtr;

            do
            {
                newCountedPtr = old;
                ++newCountedPtr.count;
            } 
            while (!atomic.compare_exchange_weak(old, newCountedPtr, 
                std::memory_order_acquire, std::memory_order_relaxed));

            old.count = newCountedPtr.count;
        }

        static void ReleaseCounter(CountedPtrElement& element)
        {
            int difference = element.count - 2;
            Counter oldCounter = element->counter.load(std::memory_order_relaxed);
            Counter newCounter;

            do
            {
                newCounter = oldCounter;
                newCounter.internalCounter += difference;
                --newCounter.externalCounters;
            } 
            while (!element->counter.compare_exchange_weak(oldCounter, newCounter,
                std::memory_order_release, std::memory_order_relaxed));

            if (newCounter.internalCounter == 0 && newCounter.externalCounters == 0)
            {
                delete element.ptr;
            }
        }

        void UpdateTail(CountedPtrElement& oldTail, CountedPtrElement& newTail)
        {
            // Trying to exchange tail only when old pointer is the same (counter can change)
            // if it is different, then the other thread already did the job, no need to continue
            auto oldPtr = oldTail.ptr;
            while (!tail.compare_exchange_weak(oldTail, newTail,
                std::memory_order_acq_rel, std::memory_order_acquire) && 
                oldTail.ptr == oldPtr);

            bool successfullyExchangedTail = oldTail.ptr == oldPtr;
            if (successfullyExchangedTail)
            {
                ReleaseCounter(oldTail);
            }
            else
            {
                oldPtr->Release();
            }
        }

        std::atomic<CountedPtrElement> head;
        std::atomic<CountedPtrElement> tail;
    };
}