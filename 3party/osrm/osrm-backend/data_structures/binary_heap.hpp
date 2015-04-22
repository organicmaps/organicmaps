/*

Copyright (c) 2015, Project OSRM contributors
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

#ifndef BINARY_HEAP_H
#define BINARY_HEAP_H

#include <boost/assert.hpp>

#include <algorithm>
#include <limits>
#include <map>
#include <type_traits>
#include <unordered_map>
#include <vector>

template <typename NodeID, typename Key> class ArrayStorage
{
  public:
    explicit ArrayStorage(size_t size) : positions(size, 0) {}

    ~ArrayStorage() {}

    Key &operator[](NodeID node) { return positions[node]; }

    Key peek_index(const NodeID node) const { return positions[node]; }

    void Clear() {}

  private:
    std::vector<Key> positions;
};

template <typename NodeID, typename Key> class MapStorage
{
  public:
    explicit MapStorage(size_t) {}

    Key &operator[](NodeID node) { return nodes[node]; }

    void Clear() { nodes.clear(); }

    Key peek_index(const NodeID node) const
    {
        const auto iter = nodes.find(node);
        if (nodes.end() != iter)
        {
            return iter->second;
        }
        return std::numeric_limits<Key>::max();
    }

  private:
    std::map<NodeID, Key> nodes;
};

template <typename NodeID, typename Key> class UnorderedMapStorage
{
  public:
    explicit UnorderedMapStorage(size_t) { nodes.rehash(1000); }

    Key &operator[](const NodeID node) { return nodes[node]; }

    Key peek_index(const NodeID node) const
    {
        const auto iter = nodes.find(node);
        if (std::end(nodes) != iter)
        {
            return iter->second;
        }
        return std::numeric_limits<Key>::max();
    }

    Key const &operator[](const NodeID node) const
    {
        auto iter = nodes.find(node);
        return iter->second;
    }

    void Clear() { nodes.clear(); }

  private:
    std::unordered_map<NodeID, Key> nodes;
};

template <typename NodeID,
          typename Key,
          typename Weight,
          typename Data,
          typename IndexStorage = ArrayStorage<NodeID, NodeID>>
class BinaryHeap
{
  private:
    BinaryHeap(const BinaryHeap &right);
    void operator=(const BinaryHeap &right);

  public:
    using WeightType = Weight;
    using DataType = Data;

    explicit BinaryHeap(size_t maxID) : node_index(maxID) { Clear(); }

    void Clear()
    {
        heap.resize(1);
        inserted_nodes.clear();
        heap[0].weight = std::numeric_limits<Weight>::min();
        node_index.Clear();
    }

    std::size_t Size() const { return (heap.size() - 1); }

    bool Empty() const { return 0 == Size(); }

    void Insert(NodeID node, Weight weight, const Data &data)
    {
        HeapElement element;
        element.index = static_cast<NodeID>(inserted_nodes.size());
        element.weight = weight;
        const Key key = static_cast<Key>(heap.size());
        heap.emplace_back(element);
        inserted_nodes.emplace_back(node, key, weight, data);
        node_index[node] = element.index;
        Upheap(key);
        CheckHeap();
    }

    Data &GetData(NodeID node)
    {
        const Key index = node_index.peek_index(node);
        return inserted_nodes[index].data;
    }

    Data const &GetData(NodeID node) const
    {
        const Key index = node_index.peek_index(node);
        return inserted_nodes[index].data;
    }

    Weight &GetKey(NodeID node)
    {
        const Key index = node_index[node];
        return inserted_nodes[index].weight;
    }

    bool WasRemoved(const NodeID node) const
    {
        BOOST_ASSERT(WasInserted(node));
        const Key index = node_index.peek_index(node);
        return inserted_nodes[index].key == 0;
    }

    bool WasInserted(const NodeID node) const
    {
        const auto index = node_index.peek_index(node);
        if (index >= static_cast<decltype(index)>(inserted_nodes.size()))
        {
            return false;
        }
        return inserted_nodes[index].node == node;
    }

    NodeID Min() const
    {
        BOOST_ASSERT(heap.size() > 1);
        return inserted_nodes[heap[1].index].node;
    }

    NodeID DeleteMin()
    {
        BOOST_ASSERT(heap.size() > 1);
        const Key removedIndex = heap[1].index;
        heap[1] = heap[heap.size() - 1];
        heap.pop_back();
        if (heap.size() > 1)
        {
            Downheap(1);
        }
        inserted_nodes[removedIndex].key = 0;
        CheckHeap();
        return inserted_nodes[removedIndex].node;
    }

    void DeleteAll()
    {
        auto iend = heap.end();
        for (typename std::vector<HeapElement>::iterator i = heap.begin() + 1; i != iend; ++i)
        {
            inserted_nodes[i->index].key = 0;
        }
        heap.resize(1);
        heap[0].weight = (std::numeric_limits<Weight>::min)();
    }

    void DecreaseKey(NodeID node, Weight weight)
    {
        BOOST_ASSERT(std::numeric_limits<NodeID>::max() != node);
        const Key &index = node_index.peek_index(node);
        Key &key = inserted_nodes[index].key;
        BOOST_ASSERT(key >= 0);

        inserted_nodes[index].weight = weight;
        heap[key].weight = weight;
        Upheap(key);
        CheckHeap();
    }

  private:
    class HeapNode
    {
      public:
        HeapNode(NodeID n, Key k, Weight w, Data d) : node(n), key(k), weight(w), data(d) {}

        NodeID node;
        Key key;
        Weight weight;
        Data data;
    };
    struct HeapElement
    {
        Key index;
        Weight weight;
    };

    std::vector<HeapNode> inserted_nodes;
    std::vector<HeapElement> heap;
    IndexStorage node_index;

    void Downheap(Key key)
    {
        const Key droppingIndex = heap[key].index;
        const Weight weight = heap[key].weight;
        const Key heap_size = static_cast<Key>(heap.size());
        Key nextKey = key << 1;
        while (nextKey < heap_size)
        {
            const Key nextKeyOther = nextKey + 1;
            if ((nextKeyOther < heap_size) && (heap[nextKey].weight > heap[nextKeyOther].weight))
            {
                nextKey = nextKeyOther;
            }
            if (weight <= heap[nextKey].weight)
            {
                break;
            }
            heap[key] = heap[nextKey];
            inserted_nodes[heap[key].index].key = key;
            key = nextKey;
            nextKey <<= 1;
        }
        heap[key].index = droppingIndex;
        heap[key].weight = weight;
        inserted_nodes[droppingIndex].key = key;
    }

    void Upheap(Key key)
    {
        const Key risingIndex = heap[key].index;
        const Weight weight = heap[key].weight;
        Key nextKey = key >> 1;
        while (heap[nextKey].weight > weight)
        {
            BOOST_ASSERT(nextKey != 0);
            heap[key] = heap[nextKey];
            inserted_nodes[heap[key].index].key = key;
            key = nextKey;
            nextKey >>= 1;
        }
        heap[key].index = risingIndex;
        heap[key].weight = weight;
        inserted_nodes[risingIndex].key = key;
    }

    void CheckHeap()
    {
#ifndef NDEBUG
        for (std::size_t i = 2; i < heap.size(); ++i)
        {
            BOOST_ASSERT(heap[i].weight >= heap[i >> 1].weight);
        }
#endif
    }
};

#endif // BINARY_HEAP_H
