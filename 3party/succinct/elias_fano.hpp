#pragma once

#include "bit_vector.hpp"
#include "darray.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

namespace succinct {

    class elias_fano {
    public:
        elias_fano()
            : m_size(0)
        {}

        struct elias_fano_builder {
            elias_fano_builder(uint64_t n, uint64_t m)
                : m_n(n)
                , m_m(m)
                , m_pos(0)
                , m_last(0)
                , m_l(uint8_t((m && n / m) ? broadword::msb(n / m) : 0))
                , m_high_bits((m + 1) + (n >> m_l) + 1)
            {
                assert(m_l < 64); // for the correctness of low_mask
                m_low_bits.reserve(m * m_l);
            }

            inline void push_back(uint64_t i) {
                assert(i >= m_last && i <= m_n);
                m_last = i;
                uint64_t low_mask = (1ULL << m_l) - 1;

                if (m_l) {
                    m_low_bits.append_bits(i & low_mask, m_l);
                }
                m_high_bits.set((i >> m_l) + m_pos, 1);
                ++m_pos;
                assert(m_pos <= m_m); (void)m_m;
            }

            friend class elias_fano;
        private:
            uint64_t m_n;
            uint64_t m_m;
            uint64_t m_pos;
            uint64_t m_last;
            uint8_t m_l;
            bit_vector_builder m_high_bits;
            bit_vector_builder m_low_bits;
        };


        elias_fano(bit_vector_builder* bvb, bool with_rank_index = true)
        {
            bit_vector_builder::bits_type& bits = bvb->move_bits();
            uint64_t n = bvb->size();

            uint64_t m = 0;
            for (size_t i = 0; i < bits.size(); ++i) {
                m += broadword::popcount(bits[i]);
            }

            bit_vector bv(bvb);
            elias_fano_builder builder(n, m);

            uint64_t i = 0;
            for (uint64_t pos = 0; pos < m; ++pos) {
                i = bv.successor1(i);
                builder.push_back(i);
                ++i;
            }

            build(builder, with_rank_index);
        }

        elias_fano(elias_fano_builder* builder, bool with_rank_index = true)
        {
            build(*builder, with_rank_index);
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_size, "m_size")
                (m_high_bits, "m_high_bits")
                (m_high_bits_d1, "m_high_bits_d1")
                (m_high_bits_d0, "m_high_bits_d0")
                (m_low_bits, "m_low_bits")
                (m_l, "m_l")
                ;
        }

        void swap(elias_fano& other) {
            std::swap(other.m_size, m_size);
            other.m_high_bits.swap(m_high_bits);
            other.m_high_bits_d1.swap(m_high_bits_d1);
            other.m_high_bits_d0.swap(m_high_bits_d0);
            other.m_low_bits.swap(m_low_bits);
            std::swap(other.m_l, m_l);
        }

        inline uint64_t size() const {
            return m_size;
        }

        inline uint64_t num_ones() const {
            return m_high_bits_d1.num_positions();
        }

        inline bool operator[](uint64_t pos) const {
            assert(pos < size());
            assert(m_high_bits_d0.num_positions()); // needs rank index
            uint64_t h_rank = pos >> m_l;
            uint64_t h_pos = m_high_bits_d0.select(m_high_bits, h_rank);
            uint64_t rank = h_pos - h_rank;
            uint64_t l_pos = pos & ((1ULL << m_l) - 1);

            while (h_pos > 0
                   && m_high_bits[h_pos - 1]) {
                --rank;
                --h_pos;
                uint64_t cur_low_bits = m_low_bits.get_bits(rank * m_l, m_l);
                if (cur_low_bits == l_pos) {
                    return true;
                } else if (cur_low_bits < l_pos) {
                    return false;
                }
            }

            return false;
        }

        inline uint64_t select(uint64_t n) const {
            return
                ((m_high_bits_d1.select(m_high_bits, n) - n) << m_l)
                | m_low_bits.get_bits(n * m_l, m_l);
        }

        inline uint64_t rank(uint64_t pos) const {
            assert(pos <= m_size);
            assert(m_high_bits_d0.num_positions()); // needs rank index
            if (pos == size()) {
                return num_ones();
            }

            uint64_t h_rank = pos >> m_l;
            uint64_t h_pos = m_high_bits_d0.select(m_high_bits, h_rank);
            uint64_t rank = h_pos - h_rank;
            uint64_t l_pos = pos & ((1ULL << m_l) - 1);

            while (h_pos > 0
                   && m_high_bits[h_pos - 1]
                   && m_low_bits.get_bits((rank - 1) * m_l, m_l) >= l_pos) {
                --rank;
                --h_pos;
            }

            return rank;
        }

        inline uint64_t predecessor1(uint64_t pos) const {
            return select(rank(pos + 1) - 1);
        }

        inline uint64_t successor1(uint64_t pos) const {
            return select(rank(pos));
        }


        // Equivalent to select(n) - select(n - 1) (and select(0) for n = 0)
        // Involves a linear search for predecessor in high bits.
        // Efficient only if there are no large gaps in high bits
        // XXX(ot): could make this adaptive
        inline uint64_t delta(uint64_t n) const {
            uint64_t high_val = m_high_bits_d1.select(m_high_bits, n);
            uint64_t low_val = m_low_bits.get_bits(n * m_l, m_l);
            if (n) {
                return
                    // need a + here instead of an | for carry
                    ((high_val - m_high_bits.predecessor1(high_val - 1) - 1) << m_l)
                    + low_val - m_low_bits.get_bits((n - 1) * m_l, m_l);
            } else {
                return
                    ((high_val - n) << m_l)
                    | low_val;
            }
        }


        // same as delta()
        inline std::pair<uint64_t, uint64_t> select_range(uint64_t n) const
        {
            assert(n + 1 < num_ones());
            uint64_t high_val_b = m_high_bits_d1.select(m_high_bits, n);
            uint64_t low_val_b = m_low_bits.get_bits(n * m_l, m_l);
            uint64_t high_val_e = m_high_bits.successor1(high_val_b + 1);
            uint64_t low_val_e = m_low_bits.get_bits((n + 1) * m_l, m_l);
            return std::make_pair(((high_val_b - n) << m_l) | low_val_b,
                                  ((high_val_e - n - 1) << m_l) | low_val_e);
        }

        struct select_enumerator {

            select_enumerator(elias_fano const& ef, uint64_t i)
                : m_ef(&ef)
                , m_i(i)
                , m_l(ef.m_l)
            {
                m_low_mask = (uint64_t(1) << m_l) - 1;
                m_low_buf = 0;
                if (m_l) {
                    m_chunks_in_word = 64 / m_l;
                    m_chunks_avail = 0;
                } else {
                    m_chunks_in_word = 0;
                    m_chunks_avail = m_ef->num_ones();
                }

                if (!m_ef->num_ones()) return;
                uint64_t pos = m_ef->m_high_bits_d1.select(m_ef->m_high_bits, m_i);
                m_high_enum =  bit_vector::unary_enumerator(m_ef->m_high_bits, pos);
                assert(m_l < 64);
            }

            uint64_t next() {
                if (!m_chunks_avail--) {
                    m_low_buf = m_ef->m_low_bits.get_word(m_i * m_l);
                    m_chunks_avail = m_chunks_in_word - 1;
                }

                uint64_t high = m_high_enum.next();
                assert(high == m_ef->m_high_bits_d1.select(m_ef->m_high_bits, m_i));
                uint64_t low = m_low_buf & m_low_mask;
                uint64_t ret =
                    ((high - m_i) << m_l)
                    | low;
                m_i += 1;
                m_low_buf >>= m_l;

                return ret;
            }

        private:

            elias_fano const* m_ef;
            uint64_t m_i;
            uint64_t m_l;
            bit_vector::unary_enumerator m_high_enum;
            uint64_t m_low_buf;
            uint64_t m_low_mask;
            uint64_t m_chunks_in_word;
            uint64_t m_chunks_avail;
        };

    protected:
        void build(elias_fano_builder& builder, bool with_rank_index) {
            m_size = builder.m_n;
            m_l = builder.m_l;
            bit_vector(&builder.m_high_bits).swap(m_high_bits);
            darray1(m_high_bits).swap(m_high_bits_d1);
            if (with_rank_index) {
                darray0(m_high_bits).swap(m_high_bits_d0);
            }
            bit_vector(&builder.m_low_bits).swap(m_low_bits);
        }

        uint64_t m_size;
        bit_vector m_high_bits;
        darray1 m_high_bits_d1;
        darray0 m_high_bits_d0;
        bit_vector m_low_bits;
        uint8_t m_l;
    };
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
