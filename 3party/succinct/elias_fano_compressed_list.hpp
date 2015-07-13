#pragma once

#include "elias_fano.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

namespace succinct {

    struct elias_fano_compressed_list
    {
        typedef uint64_t value_type;

        elias_fano_compressed_list() {}

        template <typename Range>
        elias_fano_compressed_list(Range const& ints)
        {
            typedef typename boost::range_const_iterator<Range>::type iterator_t;

            size_t s = 0;
            size_t n = 0;
            for (iterator_t iter = boost::begin(ints);
                 iter != boost::end(ints);
                 ++iter) {
                s += broadword::msb(*iter + 1);
                n += 1;
            }

            elias_fano::elias_fano_builder ef_builder(s + 1, n + 1);
            bit_vector_builder bits_builder;

            ef_builder.push_back(bits_builder.size());
            for (iterator_t iter = boost::begin(ints);
                 iter != boost::end(ints);
                 ++iter) {
                size_t val = *iter + 1;
                size_t l = broadword::msb(val);
                bits_builder.append_bits(val ^ (uint64_t(1) << l), l);
                ef_builder.push_back(bits_builder.size());
            }
            elias_fano(&ef_builder, false).swap(m_ef);
            bit_vector(&bits_builder).swap(m_bits);
        }

        value_type operator[](size_t idx) const
        {
            std::pair<size_t, size_t> r = m_ef.select_range(idx);
            size_t l = r.second - r.first;
            return ((uint64_t(1) << l) | m_bits.get_bits(r.first, l)) - 1;
        }

        size_t size() const
        {
            return m_ef.num_ones() - 1;
        }

        void swap(elias_fano_compressed_list& other)
        {
            m_ef.swap(other.m_ef);
            m_bits.swap(other.m_bits);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_ef, "m_ef")
                (m_bits, "m_bits")
                ;
        }

    private:
        elias_fano m_ef;
        bit_vector m_bits;
    };

}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
