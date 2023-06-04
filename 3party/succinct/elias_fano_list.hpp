#pragma once

#include "elias_fano.hpp"

namespace succinct {

    struct elias_fano_list
    {
        typedef uint64_t value_type;

        elias_fano_list() {}

        template <typename Range>
        elias_fano_list(Range const& ints)
        {
            typedef typename boost::range_const_iterator<Range>::type iterator_t;

            size_t s = 0;
            size_t n = 0;
            for (iterator_t iter = boost::begin(ints);
                 iter != boost::end(ints);
                 ++iter) {
                s += *iter;
                n += 1;
            }

            elias_fano::elias_fano_builder ef_builder(s + 1, n);
            size_t cur_base = 0;
            for (iterator_t iter = boost::begin(ints);
                 iter != boost::end(ints);
                 ++iter) {
                cur_base += *iter;
                ef_builder.push_back(cur_base);
            }
            elias_fano(&ef_builder, false).swap(m_ef);
        }

        value_type operator[](size_t idx) const
        {
            return m_ef.delta(idx);
        }

        size_t size() const
        {
            return m_ef.num_ones();
        }

        size_t sum() const {
            return m_ef.size() - 1;
        }

        void swap(elias_fano_list& other)
        {
            m_ef.swap(other.m_ef);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_ef, "m_ef")
                ;
        }

    private:
        elias_fano m_ef;
    };

}
