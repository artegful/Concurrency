#pragma once

#include <unordered_map>
#include <list>
#include <array>
#include <shared_mutex>
#include <algorithm>

template<typename Key, typename Value>
struct Bucket
{
public:
    using element = std::pair<Key, Value>;
    using iterator = typename std::list<element>::iterator;
    using const_iterator = typename std::list<element>::const_iterator;

    Value Get(const Key& key, const Value& defaultValue) const
    {
        std::shared_lock sharedLock(mutex);

        auto it = FindElement(key);

        return it != data.end() ? it->second : defaultValue;
    }

    void AddOrUpdate(const Key& key, const Value& value)
    {
        std::scoped_lock uniqueLock(mutex);

        auto it = FindElement(key);

        if (it == data.end())
        {
            data.emplace_back(key, value);
        }
        else
        {
            it->second = value;
        }
    }

    void Erase(const Key& key)
    {
        std::scoped_lock uniqueLock(mutex);

        auto it = FindElement(key);

        if (it != data.end())
        {
            data.erase(it);
        }
    }

    bool Empty() const
    {
        std::shared_lock sharedLock(mutex);

        return data.empty();
    }

private:
    iterator FindElement(const Key& key)
    {
        return std::find_if(data.begin(), data.end(), [&key](const element& element)
        {
            return element.first == key;
        });
    }

    const_iterator FindElement(const Key& key) const
    {
        return std::find_if(data.cbegin(), data.cend(), [&key](const element& element)
        {
            return element.first == key;
        });
    }

    mutable std::shared_mutex mutex;
    std::list<element> data;

    template<typename, typename, std::size_t, typename>
    friend class HashMap;
};

template<typename Key, typename Value, std::size_t size = 17u, typename Hash = std::hash<Key>>
class HashMap
{
public:
    HashMap() = default;
    HashMap(const Hash& hash) : hash(hash)
    { }

    Value Get(const Key& key, const Value& defaultValue = Value()) const
    {
        return data[GetIndex(key)].Get(key, defaultValue);
    }

    void AddOrUpdate(const Key& key, const Value& value)
    {
        data[GetIndex(key)].AddOrUpdate(key, value);
    }

    void Erase(const Key& key)
    {
        data[GetIndex(key)].Erase(key);
    }

    bool Empty() const
    {
        return std::all_of(data.begin(), data.end(), [](const auto& bucket)
        {
            bucket.Empty(); 
        });
    }

    std::unordered_map<Key, Value> GetCurrentState()
    {
        std::unordered_map<Key, Value> result;

        for (const auto& bucket : data)
        {
            std::shared_lock sharedLock(bucket.mutex);

            result.insert(bucket.data.begin(), bucket.data.end());
        }

        return result;
    }

private:
    std::size_t GetIndex(const Key& key) const
    {
        return hash(key) % size;
    }

    Hash hash;
    std::array<Bucket<Key, Value>, size> data;
};
