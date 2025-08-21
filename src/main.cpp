#include <thread>
#include <iostream>
#include <vector>

#include "ThreadsafeHashMap.h"
#include "LockFreeStack.h"

Stack<int> stack;
std::vector<int> results;
std::atomic<bool> start = false;

void firstThread()
{
    while (!start.load(std::memory_order_relaxed))
    {
        std::this_thread::yield();
    }

    for (int i = 0; i < 1000; i++)
    {
        stack.Push(i);
    }
}

void secondThread()
{
    while (!start.load(std::memory_order_relaxed))
    {
        std::this_thread::yield();
    }

    for (int i = 0; i < 1000000; i++)
    {
        auto res = stack.Pop();

        if (res)
        {
            results.push_back(*res);
        }
    }
}

int main()
{
    stack.Push(1);

    std::thread thread1(firstThread);
    std::thread thread2(secondThread);

    start.store(true, std::memory_order_relaxed);

    thread1.join();
    thread2.join();

    for (int num : results)
    {
        std::cout << num << ' ';
    }

    std::cout << '\n' << '\n' << results.size();

    return 0;
}