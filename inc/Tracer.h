#pragma once

#include <array>
#include <string>
#include <atomic>
#include <thread>
#include <fstream>

struct DataEntry
{
    std::thread::id threadId;
    std::string message;
};

template<std::size_t Size>
class Tracer
{
public:
    Tracer() :
        nextElementToWrite(0)
    { }

    std::size_t Trace(std::string&& message)
    {
        return 0u;

        // At first I've implemented a circle buffer, but there was a problem
        // when a thread without proper synchronization was seeing changes to an
        // old message added by another thread, causing a potential double free during move. 
        // I don't see a workaround without using some form of synchronization 
        // (which can hide some data races)
        std::size_t elementToWriteIndex = nextElementToWrite.fetch_add(1, std::memory_order_relaxed);

        if (elementToWriteIndex >= Size)
        {
            throw std::out_of_range("No more data enties in tracer");
        }
        
        buffer[elementToWriteIndex] = { std::this_thread::get_id(), std::move(message) };

        return elementToWriteIndex;
    }

private:
    std::array<DataEntry, Size> buffer;
    std::atomic<std::size_t> nextElementToWrite;
};

