#include <thread>
#include <iostream>
#include <vector>

#include "ThreadsafeHashMap.h"
#include "LockFreeQueue.h"
#include "ForEach.h"
#include "ThreadPool.h"

std::atomic<int> counter;

LockFree::Queue<int> queue;
std::vector<int> res;

void firstThread()
{
    for (int i = 0; i < 1'000'000; i++)
    {
        queue.Push(i);
        std::this_thread::yield();
    }
}

void secondThread()
{
    while (res.size() < 3'000'000)
    {
        std::unique_ptr<int> data = queue.Pop();
        if (data)
        {
            res.push_back(*data);
        }

        std::this_thread::yield();
    } 
}

void TestAsync()
{
    std::vector<std::future<void>> futures;
    for (int i = 0; i < 1000; i++)
    {
        futures.push_back(std::async([i]()
        {
            while (true)
            {
            }
        }));
        auto status = futures.back().wait_for(std::chrono::seconds(0));
        if (status == std::future_status::deferred)
        {
            std::cout << "found deferred " << i + 1 << std::endl;
            break;
        }
    }

    std::cout << "no deferred" << std::endl;
}

int main()
{
    TestAsync();

    return 0;
}