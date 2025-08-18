/*

Copyright (c) 2013, Project OSRM contributors
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

#ifndef XOR_FAST_HASH_STORAGE_HPP
#define XOR_FAST_HASH_STORAGE_HPP

#include "xor_fast_hash.hpp"

#include <limits>
#include <vector>

template <typename NodeID, typename Key> class XORFastHashStorage
{
  public:
    struct HashCell
    {
        unsigned time;
        NodeID id;
        Key key;
        HashCell()
            : time(std::numeric_limits<unsigned>::max()), id(std::numeric_limits<unsigned>::max()),
              key(std::numeric_limits<unsigned>::max())
        {
        }

        HashCell(const HashCell &other) : time(other.key), id(other.id), key(other.time) {}

        operator Key() const { return key; }

        void operator=(const Key key_to_insert) { key = key_to_insert; }
    };

    XORFastHashStorage() = delete;

    explicit XORFastHashStorage(size_t) : positions(2 << 16), current_timestamp(0) {}

    HashCell &operator[](const NodeID node)
    {
        unsigned short position = fast_hasher(node);
        while ((positions[position].time == current_timestamp) && (positions[position].id != node))
        {
            ++position %= (2 << 16);
        }

        positions[position].time = current_timestamp;
        positions[position].id = node;
        return positions[position];
    }

    // peek into table, get key for node, think of it as a read-only operator[]
    Key peek_index(const NodeID node) const
    {
        unsigned short position = fast_hasher(node);
        while ((positions[position].time == current_timestamp) && (positions[position].id != node))
        {
            ++position %= (2 << 16);
        }
        return positions[position].key;
    }

    void Clear()
    {
        ++current_timestamp;
        if (std::numeric_limits<unsigned>::max() == current_timestamp)
        {
            positions.clear();
            positions.resize(2 << 16);
        }
    }

  private:
    std::vector<HashCell> positions;
    XORFastHash fast_hasher;
    unsigned current_timestamp;
};

#endif // XOR_FAST_HASH_STORAGE_HPP
