#pragma once

#include <thread>
#include <vector>
#include <future>
#include <cmath>

#include <algorithm>
#include <iterator>
#include <functional>
#include <vector>
#include <iostream>

#include "ThreadPool.h"

inline size_t threshold = 4096;

template<typename Iter, typename Comp>
void MergeSortInternal(Iter begin, Iter end, Comp comp, unsigned int availableThreads) 
{
    auto length = std::distance(begin, end);
    if (length <= 1) 
    {
        return;
    }
    
    if (length < threshold) 
    {
        availableThreads = 1;
    }
    
    Iter mid = begin + length / 2;
    
    if (availableThreads == 1) 
    {
        MergeSortInternal(begin, mid, comp, 1);
        MergeSortInternal(mid, end, comp, 1);
    } 
    else 
    {
        auto left = availableThreads / 2;
        std::thread left_thread(MergeSortInternal<Iter, Comp>, begin, mid, comp, left);
        MergeSortInternal(mid, end, comp, left);
        left_thread.join();
    }
    
    std::inplace_merge(begin, mid, end, comp);
}

template<typename Iter, typename Comp, typename ThreadPool>
void MergeSortPoolInternal(Iter begin, Iter end, ThreadPool& pool, Comp comp) 
{
    auto length = std::distance(begin, end);
    if (length <= 1) 
    {
        return;
    }
    
    Iter mid = begin + length / 2;
     
    if (length < threshold)
    {
        MergeSortPoolInternal(begin, mid, pool, comp);
        MergeSortPoolInternal(mid, end, pool, comp);
    } 
    else 
    {
        auto leftLambda = [begin, mid, &pool, comp]()
        {
            MergeSortPoolInternal(begin, mid, pool, comp);
        };
        auto future = pool.Enqueue(leftLambda);
        MergeSortPoolInternal(mid, end, pool, comp);

        while (future.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        {
            if (!pool.TryExecuteTask())
            {
                std::this_thread::yield();
            }
        }
    }
    
    std::inplace_merge(begin, mid, end, comp);
}

template<typename Iter, typename Comp, typename Cont>
void MergeSortInternalWithBuffer(Iter begin, Iter end, Comp comp, Cont& buffer) 
{
    auto length = std::distance(begin, end);
    if (length <= 1) 
    {
        return;
    }
    
    Iter mid = begin + length / 2;
    
    MergeSortInternalWithBuffer(begin, mid, comp, buffer);
    MergeSortInternalWithBuffer(mid, end, comp, buffer);
    
    std::merge(begin, mid, mid, end, buffer.begin(), comp);
    std::copy(buffer.begin(), buffer.begin() + length, begin);
}

template<typename Iter, typename Comp, typename BufferIt>
void ParallelMergeSortInternalWithBuffer(Iter begin, Iter end, Comp comp, BufferIt bufferIt) 
{
    auto length = std::distance(begin, end);
    if (length <= 1) 
    {
        return;
    }
    
    Iter mid = begin + length / 2;
    
    if (length < threshold)
    {
        ParallelMergeSortInternalWithBuffer(begin, mid, comp, bufferIt);
        ParallelMergeSortInternalWithBuffer(mid, end, comp, bufferIt + length / 2);
    }
    else
    {
        auto future = std::async(
            ParallelMergeSortInternalWithBuffer<Iter, Comp, BufferIt>, mid, end, comp, bufferIt + length / 2);
        
        ParallelMergeSortInternalWithBuffer(begin, mid, comp, bufferIt);
        future.wait();
    }
    
    std::merge(begin, mid, mid, end, bufferIt, comp);
    std::copy(bufferIt, bufferIt + length, begin);
}

template<typename Iter, typename Comp, typename BufferIt>
void ParallelMergeSortInternalWithBufferCountThreads(Iter begin, Iter end, Comp comp, BufferIt bufferIt, std::size_t threadsAvailable) 
{
    auto length = std::distance(begin, end);
    if (length <= 1) 
    {
        return;
    }
    
    Iter mid = begin + length / 2;
    
    if (length < threshold)
    {
        ParallelMergeSortInternalWithBufferCountThreads(begin, mid, comp, bufferIt, 0);
        ParallelMergeSortInternalWithBufferCountThreads(mid, end, comp, bufferIt + length / 2, 0);
    }
    else
    {
        std::size_t leftAvailableThreads = threadsAvailable / 2;
        auto future = std::async(
            ParallelMergeSortInternalWithBufferCountThreads<Iter, Comp, BufferIt>, mid, end, 
            comp, bufferIt + length / 2, threadsAvailable - leftAvailableThreads);
        
        ParallelMergeSortInternalWithBufferCountThreads(begin, mid, comp, bufferIt, leftAvailableThreads);
        future.wait();
    }
    
    std::merge(begin, mid, mid, end, bufferIt, comp);
    std::copy(bufferIt, bufferIt + length, begin);
}

template<typename Iter, typename Comp = std::less<>>
void ParallelMergeSort(Iter begin, Iter end, Comp comp = {}) 
{
    unsigned const maxThreads = std::thread::hardware_concurrency();
    MergeSortInternal(begin, end, comp, maxThreads);
}

template<typename Iter, typename Comp = std::less<>>
void ParallelMergeSortWithBuffer(Iter begin, Iter end, Comp comp = {}) 
{
    auto length = std::distance(begin, end);
    using Value = typename std::iterator_traits<Iter>::value_type;
    std::vector<Value> buffer(length);

    ParallelMergeSortInternalWithBuffer(begin, end, comp, buffer.begin());
}

template<typename Iter, typename Comp = std::less<>>
void ParallelMergeSortWithBufferCountThreads(Iter begin, Iter end, Comp comp = {}) 
{
    auto length = std::distance(begin, end);
    using Value = typename std::iterator_traits<Iter>::value_type;
    std::vector<Value> buffer(length);

    ParallelMergeSortInternalWithBufferCountThreads(begin, end, comp, buffer.begin(), std::thread::hardware_concurrency());
}

template<typename Iter, typename Comp = std::less<>>
void MergeSort(Iter begin, Iter end, Comp comp = {})
{
    auto length = std::distance(begin, end);
    using Value = typename std::iterator_traits<Iter>::value_type;
    std::vector<Value> buffer(length);

    MergeSortInternalWithBuffer(begin, end, comp, buffer);
}

template<typename Iter, typename ThreadPool, typename Comp = std::less<>>
void ParallelMergeSortThreadPool(Iter begin, Iter end, ThreadPool& pool, Comp comp = {}) 
{
    MergeSortPoolInternal(begin, end, pool, comp);
}