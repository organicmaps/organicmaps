#pragma once

#include "broadword.hpp"
#include "forward_enumerator.hpp"
#include "darray64.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

namespace succinct {

    // Compressed random-access vector to store unsigned integers
    // using gamma codes.
    struct gamma_vector
    {
        typedef uint64_t value_type;

        gamma_vector() {}

        template <typename Range>
        gamma_vector(Range const& ints)
        {
            darray64::builder high_bits;
            bit_vector_builder low_bits;

            high_bits.append1();

            typedef typename boost::range_const_iterator<Range>::type iterator_t;
            for (iterator_t iter = boost::begin(ints);
                 iter != boost::end(ints);
                 ++iter) {
                const value_type val = *iter + 1;

                uint8_t l = broadword::msb(val);

                low_bits.append_bits(val ^ (uint64_t(1) << l), l);
                high_bits.append1(l);
            }

            darray64(&high_bits).swap(m_high_bits);
            bit_vector(&low_bits).swap(m_low_bits);
        }

        value_type operator[](uint64_t idx) const
        {
            uint64_t pos = m_high_bits.select(idx);
            uint64_t l; // ignored
            return retrieve_value(idx, pos, l);
        }

        uint64_t size() const
        {
            return m_high_bits.num_ones() - 1;
        }

        void swap(gamma_vector& other)
        {
            m_high_bits.swap(other.m_high_bits);
            m_low_bits.swap(other.m_low_bits);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_high_bits, "m_high_bits")
                (m_low_bits, "m_low_bits")
                ;
        }

    private:

        value_type retrieve_value(uint64_t idx, uint64_t pos, uint64_t & l) const
        {
            assert(m_high_bits.bits()[pos] == 1);
            l = broadword::lsb(m_high_bits.bits().get_word(pos + 1));
            return ((uint64_t(1) << l) | m_low_bits.get_bits(pos - idx, l)) - 1;
        }

        friend struct forward_enumerator<gamma_vector>;
        darray64 m_high_bits;
        bit_vector m_low_bits;
    };

    template <>
    struct forward_enumerator<gamma_vector>
    {
        typedef gamma_vector::value_type value_type;

        forward_enumerator(gamma_vector const& c, size_t idx = 0)
            : m_c(&c)
            , m_idx(idx)
            , m_pos(0)
        {
            if (idx < m_c->size()) {
                m_pos = m_c->m_high_bits.select(idx);
                m_high_bits_enumerator =
                    bit_vector::unary_enumerator(m_c->m_high_bits.bits(), m_pos + 1);
                m_low_bits_enumerator = bit_vector::enumerator(m_c->m_low_bits, m_pos - m_idx);
            }
        }

        value_type next()
        {
            assert(m_idx <= m_c->size());
            size_t next_pos = m_high_bits_enumerator.next();
            size_t l = next_pos - m_pos - 1;
            m_pos = next_pos;
            uint64_t chunk = m_low_bits_enumerator.take(l);
            uint64_t val = (chunk | (uint64_t(1) << (l))) - 1;
            m_idx += 1;
            return val;
        }

    private:
        gamma_vector const* m_c;
        uint64_t m_idx;
        uint64_t m_pos;

        bit_vector::unary_enumerator m_high_bits_enumerator;
        bit_vector::enumerator m_low_bits_enumerator;
    };
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
