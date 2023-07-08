#pragma once

#include <vector>
#include <algorithm>

#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#include "cartesian_tree.hpp"

namespace succinct {

    // XXX(ot): implement arbitrary comparator
    template <typename Vector>
    class topk_vector : boost::noncopyable {
    public:
        typedef Vector vector_type;
        typedef typename vector_type::value_type value_type;
        typedef boost::tuple<value_type, uint64_t> entry_type;
        typedef std::vector<entry_type> entry_vector_type;

        topk_vector()
        {}

        template <typename Range>
        topk_vector(Range const& v)
        {
            cartesian_tree(v, std::greater<typename boost::range_value<Range>::type>())
                .swap(m_cartesian_tree);
            vector_type(v).swap(m_v);
        }

        value_type const
        operator[](uint64_t idx) const
        {
            return m_v[idx];
        }

        uint64_t size() const
        {
            return m_v.size();
        }

        class enumerator
        {
        public:
            enumerator()
                : m_topkv(0)
            {}

            bool next()
            {
                using boost::tie;
                if (m_q.empty()) return false;

                value_type cur_mid_val;
                uint64_t cur_mid, cur_a, cur_b;

                std::pop_heap(m_q.begin(), m_q.end(), value_index_comparator());
                tie(cur_mid_val, cur_mid, cur_a, cur_b) = m_q.back();
                m_q.pop_back();

                m_cur = entry_type(cur_mid_val, cur_mid);

                if (cur_mid != cur_a) {
                    uint64_t m = m_topkv->m_cartesian_tree.rmq(cur_a, cur_mid - 1);
                    m_q.push_back(queue_element_type(m_topkv->m_v[m], m, cur_a, cur_mid - 1));
                    std::push_heap(m_q.begin(), m_q.end(), value_index_comparator());
                }

                if (cur_mid != cur_b) {
                    uint64_t m = m_topkv->m_cartesian_tree.rmq(cur_mid + 1, cur_b);
                    m_q.push_back(queue_element_type(m_topkv->m_v[m], m, cur_mid + 1, cur_b));
                    std::push_heap(m_q.begin(), m_q.end(), value_index_comparator());
                }

                return true;
            }

            entry_type const& value() const
            {
                return m_cur;
            }

            friend class topk_vector;

            void swap(enumerator& other)
            {
                using std::swap;
                swap(m_topkv, other.m_topkv);
                swap(m_q, other.m_q);
                swap(m_cur, other.m_cur);
            }

        private:

            void set(topk_vector const* topkv, uint64_t a, uint64_t b)
            {
                assert(a <= b);
                clear();
                m_topkv = topkv;

                uint64_t m = m_topkv->m_cartesian_tree.rmq(a, b);
                m_q.push_back(queue_element_type(m_topkv->m_v[m], m, a, b));
            }

            typedef boost::tuple<value_type, uint64_t, uint64_t, uint64_t> queue_element_type;

            struct value_index_comparator {
                template <typename Tuple>
                bool operator()(Tuple const& a, Tuple const& b) const
                {
                    using boost::get;
                    // lexicographic, increasing on value and decreasing
                    // on index
                    return (get<0>(a) < get<0>(b) ||
                            (get<0>(a) == get<0>(b) &&
                             get<1>(a) > get<1>(b)));
                }
            };

        public:
            void clear()
            {
                m_topkv = 0;
                m_q.clear();
            }

        private:
            topk_vector const* m_topkv;
            std::vector<queue_element_type> m_q;
            entry_type m_cur;
        };

        // NOTE this is b inclusive
        // XXX switch to [a, b) ?
        void get_topk_enumerator(uint64_t a, uint64_t b, enumerator& ret) const
        {
            ret.set(this, a, b);
        }

        enumerator get_topk_enumerator(uint64_t a, uint64_t b) const
        {
            enumerator ret;
            get_topk_enumerator(a, b, ret);
            return ret;
        }

        entry_vector_type
        topk(uint64_t a, uint64_t b, size_t k) const
        {
            entry_vector_type ret(std::min(size_t(b - a + 1), k));
            enumerator it = get_topk_enumerator(a, b);

            bool hasnext;
            for (size_t i = 0; i < ret.size(); ++i) {
                hasnext = it.next();
                assert(hasnext); (void)hasnext;
                ret[i] = it.value();
            }

            assert(ret.size() == k || !it.next());

            return ret;
        }


        template <typename Visitor>
        void map(Visitor& visit)
        {
            visit
                (m_v, "m_v")
                (m_cartesian_tree, "m_cartesian_tree");
        }

        void swap(topk_vector& other)
        {
            other.m_v.swap(m_v);
            other.m_cartesian_tree.swap(m_cartesian_tree);
        }

    protected:

        vector_type m_v;
        cartesian_tree m_cartesian_tree;
    };

}
