#pragma once

#include "broadword.hpp"
#include "forward_enumerator.hpp"
#include "darray64.hpp"

namespace succinct {

    // Compressed random-access vector to store unsigned integers
    // using gamma codes. This implementation optimizes for integers
    // whose representation is at least one bit long. It can be used,
    // for example, to represent signed integers (with uniform sign
    // distribution) by putting the sign in the LSB. For generic
    // unsigned integers, use gamma_vector

    struct gamma_bit_vector
    {
        typedef uint64_t value_type;

        gamma_bit_vector() {}

        template <typename Range>
        gamma_bit_vector(Range const& vals)
        {
            darray64::builder high_bits;
            bit_vector_builder low_bits;

            high_bits.append1();

            typedef typename boost::range_const_iterator<Range>::type iterator_t;
            for (iterator_t iter = boost::begin(vals);
                 iter != boost::end(vals);
                 ++iter) {
                const uint64_t val = *iter + 2; // increment the second bit

                uint8_t l = broadword::msb(val);

                assert(l > 0);
                high_bits.append1(l - 1);
                low_bits.append_bits(val ^ (uint64_t(1) << l), l);
            }

            darray64(&high_bits).swap(m_high_bits);
            bit_vector(&low_bits).swap(m_low_bits);
        }

        value_type operator[](size_t idx) const
        {
            size_t pos = m_high_bits.select(idx);
            size_t l; // ignored
            return retrieve_value(pos, l);
        }

        size_t size() const
        {
            return m_high_bits.num_ones() - 1;
        }

        void swap(gamma_bit_vector& other)
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

        value_type retrieve_value(size_t pos, size_t& l) const
        {
            assert(m_high_bits.bits()[pos] == 1);
            l = broadword::lsb(m_high_bits.bits().get_word(pos + 1));
            uint64_t chunk = m_low_bits.get_bits(pos, l + 1); // bit . val
            uint64_t val = ((uint64_t(1) << (l + 1)) | chunk) - 2;
            return val;
        }

        friend struct forward_enumerator<gamma_bit_vector>;

        darray64 m_high_bits;
        bit_vector m_low_bits;
    };

    template <>
    struct forward_enumerator<gamma_bit_vector>
    {
        typedef gamma_bit_vector::value_type value_type;

        forward_enumerator(gamma_bit_vector const& c, size_t idx = 0)
            : m_c(&c)
            , m_idx(idx)
            , m_pos(0)
        {
            if (idx < m_c->size()) {
                m_pos = m_c->m_high_bits.select(idx);
                m_high_bits_enumerator =
                    bit_vector::unary_enumerator(m_c->m_high_bits.bits(), m_pos + 1);
                m_low_bits_enumerator = bit_vector::enumerator(m_c->m_low_bits, m_pos);
            }
        }

        void skip(size_t k)
        {
            // XXX actually implement this
            while (k--) next();
        }

        value_type next()
        {
            assert(m_idx <= m_c->size());
            size_t next_pos = m_high_bits_enumerator.next();
            size_t l = next_pos - m_pos - 1;
            m_pos = next_pos;
            uint64_t chunk = m_low_bits_enumerator.take(l + 1);
            uint64_t val = (chunk | (uint64_t(1) << (l + 1))) - 2;
            m_idx += 1;
            return val;
        }

    private:
        gamma_bit_vector const* m_c;
        size_t m_idx;
        size_t m_pos;

        bit_vector::unary_enumerator m_high_bits_enumerator;
        bit_vector::enumerator m_low_bits_enumerator;
    };
}
