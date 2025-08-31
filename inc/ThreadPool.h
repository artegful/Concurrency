#pragma once

#include <thread>
#include <mutex>
#include <vector>
#include <deque>
#include <future>
#include <type_traits>
#include <memory>

template<typename T>
struct TaskQueue
{
public:
    TaskQueue() = default;

    template<typename Param>
    void Push(Param&& value)
    {
        std::scoped_lock lock(mutex);
        queue.push_front(std::forward<Param>(value));
    }

    bool Pop(T& value)
    {
        std::scoped_lock lock(mutex);

        if (queue.empty())
        {
            return false;
        }

        value = std::move(queue.front());
        queue.pop_front();

        return true;
    }

    bool StealPop(T& value)
    {
        std::scoped_lock lock(mutex);

        if (queue.empty())
        {
            return false;
        }

        value = std::move(queue.back());
        queue.pop_back();

        return true;
    }

    bool IsEmpty() const
    {
        std::scoped_lock lock(mutex);
        return queue.empty();
    }

    TaskQueue(const TaskQueue&) = delete;
    TaskQueue(TaskQueue&&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    TaskQueue& operator=(TaskQueue&&) = delete;
private:
    mutable std::mutex mutex;
    std::deque<T> queue;
};

class ThreadPool 
{
public:
    using StoredFunc = std::function<void()>;

    explicit ThreadPool(std::size_t size);
    bool TryExecuteTask();
    template<typename Func>
    std::future<std::invoke_result_t<Func>> Enqueue(Func task);

    bool PopFromThreadQueue(StoredFunc& func);
    bool PopFromGlobalQueue(StoredFunc& func);
    bool PopFromOtherThreadQueue(StoredFunc& func);
    
    ~ThreadPool();
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    std::vector<std::thread> workers;
    std::vector<std::unique_ptr<TaskQueue<StoredFunc>>> threadQueues;
    static thread_local TaskQueue<StoredFunc>* currentThreadQueuePtr;
    TaskQueue<std::function<void()>> globalQueue;
    std::atomic<bool> stop;
};

template<typename Func>
std::future<std::invoke_result_t<Func>> ThreadPool::Enqueue(Func task)
{
    using ReturnType = std::result_of_t<Func()>;

    // Easiest way to make packaged task copyable
    auto packagedTask = std::make_shared<std::packaged_task<ReturnType()>>(std::move(task));
    std::future<ReturnType> future = packagedTask->get_future();

    auto wrapper = [task = std::move(packagedTask)]() mutable
    {
        (*task)();
    };

    if (currentThreadQueuePtr)
    {
        currentThreadQueuePtr->Push(std::move(wrapper));
    }
    else
    {
        globalQueue.Push(std::move(wrapper));
    }

    return future;
}