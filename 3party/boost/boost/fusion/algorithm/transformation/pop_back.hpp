/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_POP_BACK_09172005_1038)
#define FUSION_POP_BACK_09172005_1038

#include <boost/fusion/view/iterator_range/iterator_range.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/end.hpp>
#include <boost/fusion/iterator/prior.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence>
        struct pop_back
        {
            typedef 
                iterator_range<
                    typename begin<Sequence>::type
                  , typename prior<
                        typename end<Sequence>::type
                    >::type
                > 
            type;
        };
    }

    template <typename Sequence>
    inline typename result_of::pop_back<Sequence const>::type
    pop_back(Sequence const& seq)
    {
        typedef typename result_of::pop_back<Sequence const>::type result;
        return result(fusion::begin(seq), fusion::prior(fusion::end(seq)));
    }
}}

#endif

