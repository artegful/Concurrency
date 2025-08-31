#pragma once

#include <thread>
#include <future>
#include <algorithm>

inline std::size_t GetOptimalAmountOfThreads(std::size_t amountOfElements)
{
    constexpr std::size_t blockSize = 256u;
    std::size_t amountOfBlocks = (amountOfElements + blockSize - 1) / blockSize;

    std::size_t hardwareThreads = std::max<std::size_t>(std::thread::hardware_concurrency(), 1u);
    return std::min<std::size_t>(hardwareThreads, amountOfBlocks);
}

template<typename It, typename Func>
void ParallelForEach(It begin, It end, Func func)
{
    std::size_t length = std::distance(begin, end);
    std::size_t amountOfThreads = GetOptimalAmountOfThreads(length);
    std::size_t blockSize = length / amountOfThreads;

    auto processingLambda = [func](It blockBegin, It blockEnd)
    {
        std::for_each(blockBegin, blockEnd, func);
    };

    std::vector<std::future<void>> futures;
    futures.reserve(amountOfThreads);

    It blockBegin = begin;
    It blockEnd = begin;

    for (std::size_t i = 0; i < amountOfThreads - 1; i++)
    {
        blockBegin = blockEnd;
        std::advance(blockEnd, blockSize);

        futures.push_back(std::async(std::launch::async, processingLambda, blockBegin, blockEnd));
    }

    processingLambda(blockEnd, end);
}