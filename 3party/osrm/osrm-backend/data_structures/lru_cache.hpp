/*

Copyright (c) 2014, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef LRUCACHE_HPP
#define LRUCACHE_HPP

#include <list>
#include <unordered_map>

template <typename KeyT, typename ValueT> class LRUCache
{
  private:
    struct CacheEntry
    {
        CacheEntry(KeyT k, ValueT v) : key(k), value(v) {}
        KeyT key;
        ValueT value;
    };
    unsigned capacity;
    std::list<CacheEntry> itemsInCache;
    std::unordered_map<KeyT, typename std::list<CacheEntry>::iterator> positionMap;

  public:
    explicit LRUCache(unsigned c) : capacity(c) {}

    bool Holds(KeyT key)
    {
        if (positionMap.find(key) != positionMap.end())
        {
            return true;
        }
        return false;
    }

    void Insert(const KeyT key, ValueT &value)
    {
        itemsInCache.push_front(CacheEntry(key, value));
        positionMap.insert(std::make_pair(key, itemsInCache.begin()));
        if (itemsInCache.size() > capacity)
        {
            positionMap.erase(itemsInCache.back().key);
            itemsInCache.pop_back();
        }
    }

    void Insert(const KeyT key, ValueT value)
    {
        itemsInCache.push_front(CacheEntry(key, value));
        positionMap.insert(std::make_pair(key, itemsInCache.begin()));
        if (itemsInCache.size() > capacity)
        {
            positionMap.erase(itemsInCache.back().key);
            itemsInCache.pop_back();
        }
    }

    bool Fetch(const KeyT key, ValueT &result)
    {
        if (Holds(key))
        {
            CacheEntry e = *(positionMap.find(key)->second);
            result = e.value;

            // move to front
            itemsInCache.splice(itemsInCache.begin(), itemsInCache, positionMap.find(key)->second);
            positionMap.find(key)->second = itemsInCache.begin();
            return true;
        }
        return false;
    }
    unsigned Size() const { return itemsInCache.size(); }
};
#endif // LRUCACHE_HPP
