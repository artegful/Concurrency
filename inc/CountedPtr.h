#pragma once

template<typename T>
struct CountedPtr
{
    T* ptr = nullptr;
    intptr_t count = 1;

    T& operator*()
    {
        return *ptr;
    }

    T* operator->()
    {
        return ptr;
    }
};