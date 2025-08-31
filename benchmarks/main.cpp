#include <benchmark/benchmark.h>
#include <array>

#include "MergeSort.h"
#include "ForEach.h"
#include "LockFreeQueue.h"
#include "ThreadsafeQueue.h"
#include "ThreadPool.h"

using Iterator = std::vector<int>::iterator;

template<void (*Algo)(Iterator, Iterator, std::less<>)>
void BM_MergeSort(benchmark::State& state) 
{
    std::vector<int> vec;
    vec.reserve(state.range(0));

    for (int i = state.range(0); i >= 0; i--)
    {
        vec.push_back(i);
    }

    for (auto _ : state)
    {
        std::vector<int> copy = vec;

        Algo(copy.begin(), copy.end(), std::less<>());

        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MergeSort<&MergeSort<Iterator>>)->Name("MergeSort")->
    RangeMultiplier(2)->Range(1 << 16, 1 << 18);
BENCHMARK(BM_MergeSort<&ParallelMergeSortWithBuffer<Iterator>>)->Name("ParallelMergeSort")->
    RangeMultiplier(2)->Range(1 << 16, 1 << 18);
BENCHMARK(BM_MergeSort<&ParallelMergeSortWithBufferCountThreads<Iterator>>)->Name("ParallelMergeSortLimitThreads")->
    RangeMultiplier(2)->Range(1 << 16, 1 << 18);

template<bool IsParallel>
void BM_ForEach(benchmark::State& state)
{
    std::vector<int> vec;
    vec.reserve(state.range(0));

    for (int i = 0; i < state.range(0); i++)
    {
        vec.push_back(i);
    }

    auto lambda = [](int& num)
    {
        num += 1;
    };

    for (auto _ : state)
    {
        std::vector<int> copy = vec;

        if constexpr (IsParallel)
        {
            ParallelForEach(copy.begin(), copy.end(), lambda);
        }
        else
        {
            std::for_each(copy.begin(), copy.end(), lambda);
        }
    }
}
BENCHMARK(BM_ForEach<true>)->Name("for_each")->RangeMultiplier(2)->Range(1 << 16, 1 << 18);
BENCHMARK(BM_ForEach<false>)->Name("ParallelForEach")->RangeMultiplier(2)->Range(1 << 16, 1 << 18);

template<typename Queue>
void PushToQueue(Queue& queue, std::size_t amount)
{
    for (std::size_t i = 0; i < amount; i++)
    {
        queue.Push(i);
    }
}

template<typename Queue>
void PopFromQueue(Queue& queue, std::size_t amount)
{
    std::size_t counter = 0;
    int previous = -1;

    while (counter < amount)
    {
        auto elementPtr = queue.Pop();

        if (elementPtr)
        {
            assert(*elementPtr > previous);
            previous = *elementPtr;
            counter++;
        }
    }
}

template<typename Queue>
void BM_Queue(benchmark::State& state)
{
    for (auto _ : state)
    {
        Queue queue;

        std::array<std::thread, 1> pushingThreads;
        for (std::size_t i = 0; i < pushingThreads.size(); i++)
        {
            pushingThreads[i] = std::thread(PushToQueue<Queue>, std::ref(queue), state.range(0) / pushingThreads.size());
        }

        std::array<std::thread, 4> popingThreads;
        for (std::size_t i = 0; i < popingThreads.size(); i++)
        {
            popingThreads[i] = std::thread(PopFromQueue<Queue>, std::ref(queue), state.range(0) / popingThreads.size());
        }

        for (auto& thread : pushingThreads)
        {
            thread.join();
        }

        for (auto& thread : popingThreads)
        {
            thread.join();
        }
        
        assert(!queue.Pop());

        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Queue<LockFree::Queue<int>>)->Name("LockfreeQueue")->RangeMultiplier(2)->Range(1 << 10, 1 << 12);
BENCHMARK(BM_Queue<Threadsafe::Queue<int>>)->Name("ThreadsafeQueue")->RangeMultiplier(2)->Range(1 << 10, 1 << 12);

struct PoolStrategy
{
    PoolStrategy() :
        pool(std::thread::hardware_concurrency())
    { }

    ThreadPool pool;

    void Run(std::vector<int>& data)
    {
        std::vector<std::future<void>> futures;
        futures.reserve(data.size());

        for (std::size_t i = 0; i < data.size(); i++)
        {
            futures.push_back(pool.Enqueue([&data, i]() { data[i]++; }));
        }

        for (std::size_t i = 0; i < futures.size(); i++)
        {
            futures[i].wait();
        }
    }
};

struct ThreadStrategy
{
    void Run(std::vector<int>& data)
    {
        std::vector<std::thread> threads;
        threads.reserve(data.size());

        for (std::size_t i = 0; i < data.size(); i++)
        {
            threads.emplace_back([&data, i]() { data[i]++; });
        }

        for (std::size_t i = 0; i < threads.size(); i++)
        {
            threads[i].join();
        }
    }
};

template<typename Strategy>
void BM_ThreadPool(benchmark::State& state)
{
    std::vector<int> vec;
    vec.reserve(state.range(0));
    Strategy strategy;

    for (int i = state.range(0); i >= 0; i--)
    {
        vec.push_back(i);
    }

    ThreadPool pool(std::thread::hardware_concurrency());

    for (auto _ : state)
    {
        std::vector<int> copy = vec;
        
        strategy.Run(copy);

        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ThreadPool<ThreadStrategy>)->Name("CreateThreads")->RangeMultiplier(2)->Range(1 << 8, 1 << 10);
BENCHMARK(BM_ThreadPool<PoolStrategy>)->Name("CreateThreadPool")->RangeMultiplier(2)->Range(1 << 8, 1 << 10);

BENCHMARK_MAIN();