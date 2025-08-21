#pragma once

#include <memory>
#include <mutex>

template<typename T>
struct Node
{
    T data;
    std::unique_ptr<Node<T>> next;
};


template<typename T>
class Queue
{
public:
    Queue()
    {
        head = std::make_unique<Node<T>>();
        tail = head.get();
    }

    bool TryPop(T& out)
    {
        std::scoped_lock headLock(headMutex);

        if (IsEmptyInternal())
        {
            return false;
        }

        std::unique_ptr<Node<T>> oldHead = PopHead();

        out = std::move_if_noexcept(oldHead->data);

        return true;
    }

    template<typename Param>
    void Push(Param&& value)
    {
        std::unique_ptr<Node<T>> newNode = std::make_unique<Node<T>>();

        std::scoped_lock tailLock(tailMutex);

        tail->data = std::forward<Param>(value);
        tail->next = std::move(newNode);
        tail = tail->next.get();
    }

    bool Empty() const
    {
        std::scoped_lock headLock(headMutex);

        return IsEmptyInternal();
    }

    Queue(const Queue<T>&) = delete;
    Queue(Queue<T>&&) = delete;
    Queue& operator=(const Queue<T>&) = delete;
    Queue& operator=(Queue<T>&&) = delete;

private:
    std::unique_ptr<Node<T>> PopHead()
    {
        std::unique_ptr<Node<T>> oldHead = std::move(head);
        head = std::move(oldHead->next);
        
        return oldHead;
    }

    Node<T>* GetTail() const
    {
        std::scoped_lock tailLock(tailMutex);

        return tail;
    }

    bool IsEmptyInternal() const
    {
        return head.get() == GetTail();
    }

    mutable std::mutex headMutex;
    mutable std::mutex tailMutex;

    std::unique_ptr<Node<T>> head;
    Node<T>* tail;
};