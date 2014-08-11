#pragma once

#include <vector>

#include <boost/range.hpp>

#include "succinct/mappable_vector.hpp"

namespace succinct {

    class nibble_vector {
    public:
        nibble_vector()
            : m_size(0)
        {}

        template <class Range>
        nibble_vector(Range const& from)
            : m_size(0)
        {
            std::vector<uint8_t> nibbles;
            bool parity = 0;
            uint8_t cur_byte = 0;
            for (typename boost::range_const_iterator<Range>::type iter = boost::begin(from);
                 iter != boost::end(from);
                 ++iter) {
                assert(*iter < 16);
                cur_byte |= *iter << (parity * 4);
                parity = !parity;
                if (!parity) {
                    nibbles.push_back(cur_byte);
                    cur_byte = 0;
                }
                ++m_size;
            }
            if (parity) {
                nibbles.push_back(cur_byte);
            }
            m_nibbles.steal(nibbles);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_size, "m_size")
                (m_nibbles, "m_nibbles");
        }

        void swap(nibble_vector& other) {
            std::swap(other.m_size, m_size);
            other.m_nibbles.swap(m_nibbles);
        }

        size_t size() const {
            return m_size;
        }

        uint8_t operator[](uint64_t pos) const {
            assert(pos < m_size);
            return (m_nibbles[pos / 2] >> ((pos % 2) * 4)) & 0x0F;
        }

    protected:
        size_t m_size;
        mapper::mappable_vector<uint8_t> m_nibbles;
    };

}
