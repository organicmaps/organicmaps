#pragma once

#include <vector>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include <boost/range.hpp>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "mappable_vector.hpp"
#include "broadword.hpp"
#include "util.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

namespace succinct {

    namespace detail {
        inline size_t words_for(uint64_t n)
        {
            return util::ceil_div(n, 64);
        }
    }

    class bit_vector;

    class bit_vector_builder : boost::noncopyable {
    public:

        typedef std::vector<uint64_t> bits_type;

        bit_vector_builder(uint64_t size = 0, bool init = 0)
            : m_size(size)
        {
            m_bits.resize(detail::words_for(size), uint64_t(-init));
            if (size) {
                m_cur_word = &m_bits.back();
                // clear padding bits
                if (init && size % 64) {
                    *m_cur_word >>= 64 - (size % 64);
                }
            }
        }

        void reserve(uint64_t size) {
            m_bits.reserve(detail::words_for(size));
        }

        inline void push_back(bool b) {
            uint64_t pos_in_word = m_size % 64;
            if (pos_in_word == 0) {
                m_bits.push_back(0);
                m_cur_word = &m_bits.back();
            }
            *m_cur_word |= (uint64_t)b << pos_in_word;
            ++m_size;
        }

        inline void set(uint64_t pos, bool b) {
            uint64_t word = pos / 64;
            uint64_t pos_in_word = pos % 64;

            m_bits[word] &= ~(uint64_t(1) << pos_in_word);
            m_bits[word] |= uint64_t(b) << pos_in_word;
        }

        inline void set_bits(uint64_t pos, uint64_t bits, size_t len)
        {
            assert(pos + len <= size());
            // check there are no spurious bits
            assert(len == 64 || (bits >> len) == 0);
            if (!len) return;
            uint64_t mask = (len == 64) ? uint64_t(-1) : ((uint64_t(1) << len) - 1);
            uint64_t word = pos / 64;
            uint64_t pos_in_word = pos % 64;

            m_bits[word] &= ~(mask << pos_in_word);
            m_bits[word] |= bits << pos_in_word;

            uint64_t stored = 64 - pos_in_word;
            if (stored < len) {
                m_bits[word + 1] &= ~(mask >> stored);
                m_bits[word + 1] |= bits >> stored;
            }
        }

        inline void append_bits(uint64_t bits, size_t len)
        {
            // check there are no spurious bits
            assert(len == 64 || (bits >> len) == 0);
            if (!len) return;
            uint64_t pos_in_word = m_size % 64;
            m_size += len;
            if (pos_in_word == 0) {
                m_bits.push_back(bits);
            } else {
                *m_cur_word |= bits << pos_in_word;
                if (len > 64 - pos_in_word) {
                    m_bits.push_back(bits >> (64 - pos_in_word));
                }
            }
            m_cur_word = &m_bits.back();
        }

        inline void zero_extend(uint64_t n) {
            m_size += n;
            uint64_t needed = detail::words_for(m_size) - m_bits.size();
            if (needed) {
                m_bits.insert(m_bits.end(), needed, 0);
                m_cur_word = &m_bits.back();
            }
        }

        inline void one_extend(uint64_t n)
        {
            while (n >= 64) {
                append_bits(uint64_t(-1), 64);
                n -= 64;
            }
            if (n) {
                append_bits(uint64_t(-1) >> (64 - n), n);
            }
        }

        void append(bit_vector_builder const& rhs)
        {
            if (!rhs.size()) return;

            uint64_t pos = m_bits.size();
            uint64_t shift = size() % 64;
            m_size = size() + rhs.size();
            m_bits.resize(detail::words_for(m_size));

            if (shift == 0) { // word-aligned, easy case
                std::copy(rhs.m_bits.begin(), rhs.m_bits.end(),
                          m_bits.begin() + ptrdiff_t(pos));
            } else {
                uint64_t* cur_word = &m_bits.front() + pos - 1;
                for (size_t i = 0; i < rhs.m_bits.size() - 1; ++i) {
                    uint64_t w = rhs.m_bits[i];
                    *cur_word |= w << shift;
                    *++cur_word = w >> (64 - shift);
                }
                *cur_word |= rhs.m_bits.back() << shift;
                if (cur_word < &m_bits.back()) {
                    *++cur_word = rhs.m_bits.back() >> (64 - shift);
                }
            }
            m_cur_word = &m_bits.back();
        }

        // reverse in place
        void reverse()
        {
            uint64_t shift = 64 - (size() % 64);

            uint64_t remainder = 0;
            for (size_t i = 0; i < m_bits.size(); ++i) {
                uint64_t cur_word;
                if (shift != 64) { // this should be hoisted out
                    cur_word = remainder | (m_bits[i] << shift);
                    remainder = m_bits[i] >> (64 - shift);
                } else {
                    cur_word = m_bits[i];
                }
                m_bits[i] = broadword::reverse_bits(cur_word);
            }
            assert(remainder == 0);
            std::reverse(m_bits.begin(), m_bits.end());
        }

        bits_type& move_bits() {
            assert(detail::words_for(m_size) == m_bits.size());
            return m_bits;
        }

        uint64_t size() const {
            return m_size;
        }

        void swap(bit_vector_builder& other)
        {
            m_bits.swap(other.m_bits);
            std::swap(m_size, other.m_size);
            std::swap(m_cur_word, other.m_cur_word);
        }

    private:
        bits_type m_bits;
        uint64_t m_size;
        uint64_t* m_cur_word;
    };

    class bit_vector {
    public:
        bit_vector()
            : m_size(0)
        {}

        template <class Range>
        bit_vector(Range const& from) {
            std::vector<uint64_t> bits;
            const uint64_t first_mask = uint64_t(1);
            uint64_t mask = first_mask;
            uint64_t cur_val = 0;
            m_size = 0;
            for (typename boost::range_const_iterator<Range>::type iter = boost::begin(from);
                 iter != boost::end(from);
                 ++iter) {
                if (*iter) {
                    cur_val |= mask;
                }
                mask <<= 1;
                m_size += 1;
                if (!mask) {
                    bits.push_back(cur_val);
                    mask = first_mask;
                    cur_val = 0;
                }
            }
            if (mask != first_mask) {
                bits.push_back(cur_val);
            }
            m_bits.steal(bits);
        }

        bit_vector(bit_vector_builder* from) {
            m_size = from->size();
            m_bits.steal(from->move_bits());
        }

        template <typename Visitor>
        void map(Visitor& visit) {
            visit
                (m_size, "m_size")
                (m_bits, "m_bits");
        }

        void swap(bit_vector& other) {
            std::swap(other.m_size, m_size);
            other.m_bits.swap(m_bits);
        }

        inline uint64_t size() const {
            return m_size;
        }

        inline bool operator[](uint64_t pos) const {
            assert(pos < m_size);
            uint64_t block = pos / 64;
            assert(block < m_bits.size());
            uint64_t shift = pos % 64;
            return (m_bits[block] >> shift) & 1;
        }

        inline uint64_t get_bits(uint64_t pos, uint64_t len) const {
            assert(pos + len <= size());
            if (!len) {
                return 0;
            }
            uint64_t block = pos / 64;
            uint64_t shift = pos % 64;
            uint64_t mask = -(len == 64) | ((1ULL << len) - 1);
            if (shift + len <= 64) {
                return m_bits[block] >> shift & mask;
            } else {
                return (m_bits[block] >> shift) | (m_bits[block + 1] << (64 - shift) & mask);
            }
        }

        // same as get_bits(pos, 64) but it can extend further size(), padding with zeros
        inline uint64_t get_word(uint64_t pos) const
        {
            assert(pos < size());
            uint64_t block = pos / 64;
            uint64_t shift = pos % 64;
            uint64_t word = m_bits[block] >> shift;
            if (shift && block + 1 < m_bits.size()) {
                word |= m_bits[block + 1] << (64 - shift);
            }
            return word;
        }

        // unsafe and fast version of get_word, it retrieves at least 56 bits
        /*
        inline uint64_t get_word56(uint64_t pos) const
        {
            // XXX check endianness?
            const char* ptr = reinterpret_cast<const char*>(m_bits.data());
            return *(reinterpret_cast<uint64_t const*>(ptr + pos / 8)) >> (pos % 8);
        }
        */

        inline uint64_t predecessor0(uint64_t pos) const {
            assert(pos < m_size);
            uint64_t block = pos / 64;
            uint64_t shift = 64 - pos % 64 - 1;
            uint64_t word = ~m_bits[block];
            word = (word << shift) >> shift;

            unsigned long ret;
            while (!broadword::msb(word, ret)) {
                assert(block);
                word = ~m_bits[--block];
            };
            return block * 64 + ret;
        }

        inline uint64_t successor0(uint64_t pos) const {
            assert(pos < m_size);
            uint64_t block = pos / 64;
            uint64_t shift = pos % 64;
            uint64_t word = (~m_bits[block] >> shift) << shift;

            unsigned long ret;
            while (!broadword::lsb(word, ret)) {
                ++block;
                assert(block < m_bits.size());
                word = ~m_bits[block];
            };
            return block * 64 + ret;
        }

        inline uint64_t predecessor1(uint64_t pos) const {
            assert(pos < m_size);
            uint64_t block = pos / 64;
            uint64_t shift = 64 - pos % 64 - 1;
            uint64_t word = m_bits[block];
            word = (word << shift) >> shift;

            unsigned long ret;
            while (!broadword::msb(word, ret)) {
                assert(block);
                word = m_bits[--block];
            };
            return block * 64 + ret;
        }

        inline uint64_t successor1(uint64_t pos) const {
            assert(pos < m_size);
            uint64_t block = pos / 64;
            uint64_t shift = pos % 64;
            uint64_t word = (m_bits[block] >> shift) << shift;

            unsigned long ret;
            while (!broadword::lsb(word, ret)) {
                ++block;
                assert(block < m_bits.size());
                word = m_bits[block];
            };
            return block * 64 + ret;
        }

        mapper::mappable_vector<uint64_t> const& data() const
        {
            return m_bits;
        }

        struct enumerator {
            enumerator()
                : m_bv(0)
                , m_pos(uint64_t(-1))
            {}

            enumerator(bit_vector const& bv, uint64_t pos)
                : m_bv(&bv)
                , m_pos(pos)
                , m_buf(0)
                , m_avail(0)
            {
                m_bv->data().prefetch(m_pos / 64);
            }

            inline bool next()
            {
                if (!m_avail) fill_buf();
                bool b = m_buf & 1;
                m_buf >>= 1;
                m_avail -= 1;
                m_pos += 1;
                return b;
            }

            inline uint64_t take(uint64_t l)
            {
                if (m_avail < l) fill_buf();
                uint64_t val;
                if (l != 64) {
                    val = m_buf & ((uint64_t(1) << l) - 1);
                    m_buf >>= l;
                } else {
                    val = m_buf;
                }
                m_avail -= l;
                m_pos += l;
                return val;
            }

            inline uint64_t skip_zeros()
            {
                uint64_t zs = 0;
                // XXX the loop may be optimized by aligning access
                while (!m_buf) {
                    m_pos += m_avail;
                    zs += m_avail;
                    m_avail = 0;
                    fill_buf();
                }

                uint64_t l = broadword::lsb(m_buf);
                m_buf >>= l;
                m_buf >>= 1;
                m_avail -= l + 1;
                m_pos += l + 1;
                return zs + l;
            }

            inline uint64_t position() const
            {
                return m_pos;
            }

        private:

            inline void fill_buf()
            {
                m_buf = m_bv->get_word(m_pos);
                m_avail = 64;
            }

            bit_vector const* m_bv;
            uint64_t m_pos;
            uint64_t m_buf;
            uint64_t m_avail;
        };

        struct unary_enumerator {
            unary_enumerator()
                : m_data(0)
                , m_position(0)
                , m_buf(0)
            {}

            unary_enumerator(bit_vector const& bv, uint64_t pos)
            {
                m_data = bv.data().data();
                m_position = pos;
                m_buf = m_data[pos / 64];
                // clear low bits
                m_buf &= uint64_t(-1) << (pos % 64);
            }

            uint64_t position() const
            {
                return m_position;
            }

            uint64_t next()
            {
                unsigned long pos_in_word;
                uint64_t buf = m_buf;
                while (!broadword::lsb(buf, pos_in_word)) {
                    m_position += 64;
                    buf = m_data[m_position / 64];
                }

                m_buf = buf & (buf - 1); // clear LSB
                m_position = (m_position & ~uint64_t(63)) + pos_in_word;
                return m_position;
            }

            // skip to the k-th one after the current position
            void skip(uint64_t k)
            {
                uint64_t skipped = 0;
                uint64_t buf = m_buf;
                uint64_t w = 0;
                while (skipped + (w = broadword::popcount(buf)) <= k) {
                    skipped += w;
                    m_position += 64;
                    buf = m_data[m_position / 64];
                }
                assert(buf);
                uint64_t pos_in_word = broadword::select_in_word(buf, k - skipped);
                m_buf = buf & (uint64_t(-1) << pos_in_word);
                m_position = (m_position & ~uint64_t(63)) + pos_in_word;
            }

            // skip to the k-th zero after the current position
            void skip0(uint64_t k)
            {
                uint64_t skipped = 0;
                uint64_t pos_in_word = m_position % 64;
                uint64_t buf = ~m_buf & (uint64_t(-1) << pos_in_word);
                uint64_t w = 0;
                while (skipped + (w = broadword::popcount(buf)) <= k) {
                    skipped += w;
                    m_position += 64;
                    buf = ~m_data[m_position / 64];
                }
                assert(buf);
                pos_in_word = broadword::select_in_word(buf, k - skipped);
                m_buf = ~buf & (uint64_t(-1) << pos_in_word);
                m_position = (m_position & ~uint64_t(63)) + pos_in_word;
            }

        private:
            uint64_t const* m_data;
            uint64_t m_position;
            uint64_t m_buf;
        };

    protected:
        uint64_t m_size;
        mapper::mappable_vector<uint64_t> m_bits;
    };

}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
