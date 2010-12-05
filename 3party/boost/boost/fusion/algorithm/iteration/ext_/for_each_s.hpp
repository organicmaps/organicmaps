/*=============================================================================
    Copyright (c) 2006 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_FOR_EACH_S_05022006_1027)
#define FUSION_FOR_EACH_S_05022006_1027

#include <boost/mpl/assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/sequence/intrinsic/ext_/segments.hpp>
#include <boost/fusion/support/ext_/is_segmented.hpp>

// fwd declarations
namespace boost { namespace fusion
{
    template <typename Sequence, typename F>
    void
    for_each_s(Sequence& seq, F const& f);

    template <typename Sequence, typename F>
    void
    for_each_s(Sequence const& seq, F const& f);
}}

namespace boost { namespace fusion { namespace detail
{
    template<typename F>
    struct for_each_s_bind
    {
        explicit for_each_s_bind(F const &f)
          : f_(f)
        {}

        template<typename Sequence>
        void operator ()(Sequence &seq) const
        {
            fusion::for_each_s(seq, this->f_);
        }

        template<typename Sequence>
        void operator ()(Sequence const &seq) const
        {
            fusion::for_each_s(seq, this->f_);
        }
    private:
        F const &f_;
    };

    template<typename Sequence, typename F>
    void for_each_s(Sequence &seq, F const &f, mpl::true_)
    {
        fusion::for_each_s(fusion::segments(seq), for_each_s_bind<F>(f));
    }

    template<typename Sequence, typename F>
    void for_each_s(Sequence &seq, F const &f, mpl::false_)
    {
        fusion::for_each(seq, f);
    }
}}}

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence, typename F>
        struct for_each_s
        {
            typedef void type;
        };
    }

    template <typename Sequence, typename F>
    inline void
    for_each_s(Sequence& seq, F const& f)
    {
        detail::for_each_s(seq, f, traits::is_segmented<Sequence>());
    }

    template <typename Sequence, typename F>
    inline void
    for_each_s(Sequence const& seq, F const& f)
    {
        detail::for_each_s(seq, f, traits::is_segmented<Sequence>());
    }
}}

#endif
