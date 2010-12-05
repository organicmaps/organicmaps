/*=============================================================================
    Copyright (c) 2006 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_SEGMENTS_04052005_1141)
#define FUSION_SEGMENTS_04052005_1141

#include <boost/fusion/support/tag_of.hpp>

namespace boost { namespace fusion
{
    // segments: returns a sequence of sequences
    namespace extension
    {
        template <typename Tag>
        struct segments_impl
        {
            template <typename Sequence>
            struct apply {};
        };
    }

    namespace result_of
    {
        template <typename Sequence>
        struct segments
        {
            typedef typename
                extension::segments_impl<typename traits::tag_of<Sequence>::type>::
                    template apply<Sequence>::type
            type;
        };
    }

    template <typename Sequence>
    typename result_of::segments<Sequence>::type
    segments(Sequence & seq)
    {
        return
            extension::segments_impl<typename traits::tag_of<Sequence>::type>::
                template apply<Sequence>::call(seq);
    }

    template <typename Sequence>
    typename result_of::segments<Sequence const>::type
    segments(Sequence const& seq)
    {
        return
            extension::segments_impl<typename traits::tag_of<Sequence>::type>::
                template apply<Sequence const>::call(seq);
    }
}}

#endif
