#include "ThreadPool.h"

thread_local TaskQueue<ThreadPool::StoredFunc>* ThreadPool::currentThreadQueuePtr;

ThreadPool::ThreadPool(std::size_t size) : 
    stop(false) 
{
    auto workerStart = [this](std::size_t index)
    {
        currentThreadQueuePtr = threadQueues[index].get();

        while (!stop.load(std::memory_order_relaxed)) 
        {
            if (!TryExecuteTask())
            {
                std::this_thread::yield();
            }
        }
    };

    try
    {
        threadQueues.resize(size);

        // Have to separate them, so creation of all queues synchronizes with every thread start 
        for (size_t i = 0; i < size; i++)
        {
            threadQueues[i] = std::make_unique<TaskQueue<StoredFunc>>();
        }

        for (size_t i = 0; i < size; i++)
        {
            workers.emplace_back(workerStart, i);
        }
    }
    catch(const std::exception& e)
    {
        stop.store(true, std::memory_order_relaxed);
    }
}

bool ThreadPool::PopFromThreadQueue(StoredFunc& func)
{
    if (!currentThreadQueuePtr)
    {
        return false;
    }

    return currentThreadQueuePtr->Pop(func);
}

bool ThreadPool::PopFromGlobalQueue(StoredFunc& func)
{
    return globalQueue.Pop(func);
}

bool ThreadPool::PopFromOtherThreadQueue(StoredFunc& func)
{
    for (std::size_t i = 0; i < threadQueues.size(); i++)
    {
        if (threadQueues[i]->Pop(func))
        {
            return true;
        }
    }

    return false;
}

bool ThreadPool::TryExecuteTask()
{
    std::function<void()> task;

    if (PopFromThreadQueue(task) || PopFromGlobalQueue(task) || PopFromOtherThreadQueue(task))
    {
        task();
        return true;
    }

    return false;
}

ThreadPool::~ThreadPool() 
{
    stop.store(true);

    for (std::thread& worker : workers)
    {
        worker.join();
    }
}