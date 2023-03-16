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

#ifndef DEALLOCATING_VECTOR_HPP
#define DEALLOCATING_VECTOR_HPP

#include "../util/integer_range.hpp"

#include <boost/iterator/iterator_facade.hpp>

#include <limits>
#include <utility>
#include <vector>

template <typename ElementT> struct DeallocatingVectorIteratorState
{
    DeallocatingVectorIteratorState()
        : index(std::numeric_limits<std::size_t>::max()), bucket_list(nullptr)
    {
    }
    explicit DeallocatingVectorIteratorState(const DeallocatingVectorIteratorState &r)
        : index(r.index), bucket_list(r.bucket_list)
    {
    }
    explicit DeallocatingVectorIteratorState(const std::size_t idx,
                                             std::vector<ElementT *> *input_list)
        : index(idx), bucket_list(input_list)
    {
    }
    std::size_t index;
    std::vector<ElementT *> *bucket_list;

    DeallocatingVectorIteratorState &operator=(const DeallocatingVectorIteratorState &other)
    {
        index = other.index;
        bucket_list = other.bucket_list;
        return *this;
    }
};

template <typename ElementT, std::size_t ELEMENTS_PER_BLOCK>
class DeallocatingVectorIterator
    : public boost::iterator_facade<DeallocatingVectorIterator<ElementT, ELEMENTS_PER_BLOCK>,
                                    ElementT,
                                    std::random_access_iterator_tag>
{
    DeallocatingVectorIteratorState<ElementT> current_state;

  public:
    DeallocatingVectorIterator() {}
    DeallocatingVectorIterator(std::size_t idx, std::vector<ElementT *> *input_list)
        : current_state(idx, input_list)
    {
    }

    friend class boost::iterator_core_access;

    void advance(std::size_t n) { current_state.index += n; }

    void increment() { advance(1); }

    void decrement() { advance(-1); }

    bool equal(DeallocatingVectorIterator const &other) const
    {
        return current_state.index == other.current_state.index;
    }

    std::ptrdiff_t distance_to(DeallocatingVectorIterator const &other) const
    {
        // it is important to implement it 'other minus this'. otherwise sorting breaks
        return other.current_state.index - current_state.index;
    }

    ElementT &dereference() const
    {
        const std::size_t current_bucket = current_state.index / ELEMENTS_PER_BLOCK;
        const std::size_t current_index = current_state.index % ELEMENTS_PER_BLOCK;
        return (current_state.bucket_list->at(current_bucket)[current_index]);
    }

    ElementT &operator[](const std::size_t index) const
    {
        const std::size_t current_bucket = (index + current_state.index) / ELEMENTS_PER_BLOCK;
        const std::size_t current_index = (index + current_state.index) % ELEMENTS_PER_BLOCK;
        return (current_state.bucket_list->at(current_bucket)[current_index]);
    }
};

template <typename ElementT, std::size_t ELEMENTS_PER_BLOCK>
class DeallocatingVectorRemoveIterator
    : public boost::iterator_facade<DeallocatingVectorRemoveIterator<ElementT, ELEMENTS_PER_BLOCK>,
                                    ElementT,
                                    boost::forward_traversal_tag>
{
    DeallocatingVectorIteratorState<ElementT> current_state;

  public:
    DeallocatingVectorRemoveIterator(std::size_t idx, std::vector<ElementT *> *input_list)
        : current_state(idx, input_list)
    {
    }

    friend class boost::iterator_core_access;

    void increment()
    {
        const std::size_t old_bucket = current_state.index / ELEMENTS_PER_BLOCK;

        ++current_state.index;
        const std::size_t new_bucket = current_state.index / ELEMENTS_PER_BLOCK;
        if (old_bucket != new_bucket)
        {
            // delete old bucket entry
            if (nullptr != current_state.bucket_list->at(old_bucket))
            {
                delete[] current_state.bucket_list->at(old_bucket);
                current_state.bucket_list->at(old_bucket) = nullptr;
            }
        }
    }

    bool equal(DeallocatingVectorRemoveIterator const &other) const
    {
        return current_state.index == other.current_state.index;
    }

    std::ptrdiff_t distance_to(DeallocatingVectorRemoveIterator const &other) const
    {
        return other.current_state.index - current_state.index;
    }

    ElementT &dereference() const
    {
        const std::size_t current_bucket = current_state.index / ELEMENTS_PER_BLOCK;
        const std::size_t current_index = current_state.index % ELEMENTS_PER_BLOCK;
        return (current_state.bucket_list->at(current_bucket)[current_index]);
    }
};

template <typename ElementT, std::size_t ELEMENTS_PER_BLOCK = 8388608 / sizeof(ElementT)>
class DeallocatingVector
{
    std::size_t current_size;
    std::vector<ElementT *> bucket_list;

  public:
    using iterator = DeallocatingVectorIterator<ElementT, ELEMENTS_PER_BLOCK>;
    using const_iterator = DeallocatingVectorIterator<ElementT, ELEMENTS_PER_BLOCK>;

    // this forward-only iterator deallocates all buckets that have been visited
    using deallocation_iterator = DeallocatingVectorRemoveIterator<ElementT, ELEMENTS_PER_BLOCK>;

    DeallocatingVector() : current_size(0)
    {
        bucket_list.emplace_back(new ElementT[ELEMENTS_PER_BLOCK]);
    }

    ~DeallocatingVector() { clear(); }

    void swap(DeallocatingVector<ElementT, ELEMENTS_PER_BLOCK> &other)
    {
        std::swap(current_size, other.current_size);
        bucket_list.swap(other.bucket_list);
    }

    void clear()
    {
        // Delete[]'ing ptr's to all Buckets
        for (auto bucket : bucket_list)
        {
            if (nullptr != bucket)
            {
                delete[] bucket;
                bucket = nullptr;
            }
        }
        bucket_list.clear();
        bucket_list.shrink_to_fit();
        current_size = 0;
    }

    void push_back(const ElementT &element)
    {
        const std::size_t current_capacity = capacity();
        if (current_size == current_capacity)
        {
            bucket_list.push_back(new ElementT[ELEMENTS_PER_BLOCK]);
        }

        std::size_t current_index = size() % ELEMENTS_PER_BLOCK;
        bucket_list.back()[current_index] = element;
        ++current_size;
    }

    template <typename... Ts> void emplace_back(Ts &&... element)
    {
        const std::size_t current_capacity = capacity();
        if (current_size == current_capacity)
        {
            bucket_list.push_back(new ElementT[ELEMENTS_PER_BLOCK]);
        }

        const std::size_t current_index = size() % ELEMENTS_PER_BLOCK;
        bucket_list.back()[current_index] = ElementT(std::forward<Ts>(element)...);
        ++current_size;
    }

    void reserve(const std::size_t) const { /* don't do anything */}

    void resize(const std::size_t new_size)
    {
        if (new_size >= current_size)
        {
            while (capacity() < new_size)
            {
                bucket_list.push_back(new ElementT[ELEMENTS_PER_BLOCK]);
            }
        }
        else
        { // down-size
            const std::size_t number_of_necessary_buckets = 1 + (new_size / ELEMENTS_PER_BLOCK);
            for (const auto bucket_index :
                 osrm::irange(number_of_necessary_buckets, bucket_list.size()))
            {
                if (nullptr != bucket_list[bucket_index])
                {
                    delete[] bucket_list[bucket_index];
                }
            }
            bucket_list.resize(number_of_necessary_buckets);
        }
        current_size = new_size;
    }

    std::size_t size() const { return current_size; }

    std::size_t capacity() const { return bucket_list.size() * ELEMENTS_PER_BLOCK; }

    iterator begin() { return iterator(static_cast<std::size_t>(0), &bucket_list); }

    iterator end() { return iterator(size(), &bucket_list); }

    deallocation_iterator dbegin()
    {
        return deallocation_iterator(static_cast<std::size_t>(0), &bucket_list);
    }

    deallocation_iterator dend() { return deallocation_iterator(size(), &bucket_list); }

    const_iterator begin() const
    {
        return const_iterator(static_cast<std::size_t>(0), &bucket_list);
    }

    const_iterator end() const { return const_iterator(size(), &bucket_list); }

    ElementT &operator[](const std::size_t index)
    {
        const std::size_t _bucket = index / ELEMENTS_PER_BLOCK;
        const std::size_t _index = index % ELEMENTS_PER_BLOCK;
        return (bucket_list[_bucket][_index]);
    }

    ElementT &operator[](const std::size_t index) const
    {
        const std::size_t _bucket = index / ELEMENTS_PER_BLOCK;
        const std::size_t _index = index % ELEMENTS_PER_BLOCK;
        return (bucket_list[_bucket][_index]);
    }

    ElementT &back() const
    {
        const std::size_t _bucket = current_size / ELEMENTS_PER_BLOCK;
        const std::size_t _index = current_size % ELEMENTS_PER_BLOCK;
        return (bucket_list[_bucket][_index]);
    }

    template <class InputIterator> void append(InputIterator first, const InputIterator last)
    {
        InputIterator position = first;
        while (position != last)
        {
            push_back(*position);
            ++position;
        }
    }
};

#endif /* DEALLOCATING_VECTOR_HPP */
