#include <atomic>
#include <type_traits>
#include <memory>

template<typename T>
struct CountedPtr
{
    T* ptr = nullptr;
    int count = 1;

    T& operator*()
    {
        return *ptr;
    }

    T* operator->()
    {
        return ptr;
    }
};

template<typename T>
struct Node
{
    std::unique_ptr<T> data;
    CountedPtr<Node<T>> next;
    std::atomic<int> internalCount;
};

template<typename T>
class Stack
{
public:
    using CountedPtrElement = CountedPtr<Node<T>>;

    void Push(const T& value)
    {
        Node<T>* node = new Node<T>();
        node->data = std::make_unique<T>(value);
        node->next = head.load(std::memory_order_relaxed);
        CountedPtrElement countedPtr = { node, 1 };

        while (!head.compare_exchange_weak(node->next, countedPtr, std::memory_order_release, std::memory_order_relaxed));
    }

    std::unique_ptr<T> Pop()
    {
        while (true)
        {
            CountedPtrElement old = head.load(std::memory_order_relaxed);
            IncreaseRefCount(old);
            
            if (old.ptr && head.compare_exchange_strong(old, old->next, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                std::unique_ptr<T> result = std::move(old->data);

                int difference = old.count - 2;
                if (old->internalCount.fetch_add(diff, std::memory_order_release) == -diff)
                {
                    delete old.ptr;
                }

                return result;
            }

            if (!old.ptr)
            {
                return {};
            }

            if (old->internalCount.fetch_sub(1, std::memory_order_acquire) == 1)
            {
                delete old.ptr;
            }
        }
    }

private:
    void IncreaseRefCount(CountedPtrElement& old)
    {
        CountedPtrElement newCountedPtr;

        do
        {
            newCountedPtr = old;
            ++newCountedPtr.count;
        } 
        while (!head.compare_exchange_weak(old, newCountedPtr, std::memory_order_acquire, std::memory_order_relaxed));

        old.count = newCountedPtr.count;
    }

    void TryDelete(Node<T>* ptr)
    {

    }

    std::atomic<CountedPtrElement> head;
};