/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2008 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PROTO_DETAIL_FUSION_POP_FRONT_EAH_01_22_2008
#define BOOST_PROTO_DETAIL_FUSION_POP_FRONT_EAH_01_22_2008

#include <boost/spirit/fusion/sequence/range.hpp>
#include <boost/spirit/fusion/sequence/begin.hpp>
#include <boost/spirit/fusion/sequence/end.hpp>
#include <boost/spirit/fusion/iterator/next.hpp>

namespace boost { namespace fusion
{
    namespace meta
    {
        template <typename Sequence>
        struct pop_front
        {
            typedef
                range<
                    typename next<
                        typename begin<Sequence>::type
                    >::type
                  , typename end<Sequence>::type
                >
            type;
        };
    }

    template <typename Sequence>
    inline typename meta::pop_front<Sequence const>::type
    pop_front(Sequence const& seq)
    {
        typedef typename meta::pop_front<Sequence const>::type result;
        return result(fusion::next(fusion::begin(seq)), fusion::end(seq));
    }
}}

#endif
