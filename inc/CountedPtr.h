#pragma once

template<typename T>
struct CountedPtr
{
    T* ptr = nullptr;
    long long count = 1;

    T& operator*()
    {
        return *ptr;
    }

    T* operator->()
    {
        return ptr;
    }

    static_assert(sizeof(T*) == sizeof(long long));
};