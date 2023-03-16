#pragma once

namespace succinct {

    template <typename Container>
    struct forward_enumerator
    {
        typedef typename Container::value_type value_type;

        forward_enumerator(Container const& c, size_t idx = 0)
            : m_c(&c)
            , m_idx(idx)
        {}

        value_type next()
        {
            return (*m_c)[m_idx++];
        }

    private:
        Container const* m_c;
        size_t m_idx;
    };

}
