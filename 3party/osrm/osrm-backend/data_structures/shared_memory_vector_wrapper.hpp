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

#ifndef SHARED_MEMORY_VECTOR_WRAPPER_HPP
#define SHARED_MEMORY_VECTOR_WRAPPER_HPP

#include <boost/assert.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <vector>

template <typename DataT> class ShMemIterator : public std::iterator<std::input_iterator_tag, DataT>
{
    DataT *p;

  public:
    explicit ShMemIterator(DataT *x) : p(x) {}
    ShMemIterator(const ShMemIterator &mit) : p(mit.p) {}
    ShMemIterator &operator++()
    {
        ++p;
        return *this;
    }
    ShMemIterator operator++(int)
    {
        ShMemIterator tmp(*this);
        operator++();
        return tmp;
    }
    ShMemIterator operator+(std::ptrdiff_t diff)
    {
        ShMemIterator tmp(p + diff);
        return tmp;
    }
    bool operator==(const ShMemIterator &rhs) { return p == rhs.p; }
    bool operator!=(const ShMemIterator &rhs) { return p != rhs.p; }
    DataT &operator*() { return *p; }
};

template <typename DataT> class SharedMemoryWrapper
{
  private:
    DataT *m_ptr;
    std::size_t m_size;

  public:
    SharedMemoryWrapper() : m_ptr(nullptr), m_size(0) {}

    SharedMemoryWrapper(DataT *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

    void swap(SharedMemoryWrapper<DataT> &other)
    {
        // BOOST_ASSERT_MSG(m_size != 0 || other.size() != 0, "size invalid");
        std::swap(m_size, other.m_size);
        std::swap(m_ptr, other.m_ptr);
    }

    DataT &at(const std::size_t index) { return m_ptr[index]; }

    const DataT &at(const std::size_t index) const { return m_ptr[index]; }

    ShMemIterator<DataT> begin() const { return ShMemIterator<DataT>(m_ptr); }

    ShMemIterator<DataT> end() const { return ShMemIterator<DataT>(m_ptr + m_size); }

    std::size_t size() const { return m_size; }

    bool empty() const { return 0 == size(); }

    DataT &operator[](const unsigned index)
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        return m_ptr[index];
    }

    const DataT &operator[](const unsigned index) const
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        return m_ptr[index];
    }
};

template <> class SharedMemoryWrapper<bool>
{
  private:
    unsigned *m_ptr;
    std::size_t m_size;

  public:
    SharedMemoryWrapper() : m_ptr(nullptr), m_size(0) {}

    SharedMemoryWrapper(unsigned *ptr, std::size_t size) : m_ptr(ptr), m_size(size) {}

    void swap(SharedMemoryWrapper<bool> &other)
    {
        // BOOST_ASSERT_MSG(m_size != 0 || other.size() != 0, "size invalid");
        std::swap(m_size, other.m_size);
        std::swap(m_ptr, other.m_ptr);
    }

    bool at(const std::size_t index) const
    {
        const std::size_t bucket = index / 32;
        const unsigned offset = static_cast<unsigned>(index % 32);
        return m_ptr[bucket] & (1 << offset);
    }

    std::size_t size() const { return m_size; }

    bool empty() const { return 0 == size(); }

    bool operator[](const unsigned index)
    {
        BOOST_ASSERT_MSG(index < m_size, "invalid size");
        const unsigned bucket = index / 32;
        const unsigned offset = index % 32;
        return m_ptr[bucket] & (1 << offset);
    }
};

template <typename DataT, bool UseSharedMemory> struct ShM
{
    using vector = typename std::conditional<UseSharedMemory,
                                             SharedMemoryWrapper<DataT>,
                                             std::vector<DataT>>::type;
};

#endif // SHARED_MEMORY_VECTOR_WRAPPER_HPP
