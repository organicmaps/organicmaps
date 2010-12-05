/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONVERT_20061213_2207)
#define FUSION_CONVERT_20061213_2207

#include <boost/fusion/container/deque/detail/as_deque.hpp>
#include <boost/fusion/container/deque/detail/convert_impl.hpp>
#include <boost/fusion/container/deque/deque.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>

namespace boost { namespace fusion
{
    namespace result_of
    {
        template <typename Sequence>
        struct as_deque
        {
            typedef typename detail::as_deque<result_of::size<Sequence>::value> gen;
            typedef typename gen::
                template apply<typename result_of::begin<Sequence>::type>::type
            type;
        };
    }

    template <typename Sequence>
    inline typename result_of::as_deque<Sequence>::type
    as_deque(Sequence& seq)
    {
        typedef typename result_of::as_deque<Sequence>::gen gen;
        return gen::call(fusion::begin(seq));
    }

    template <typename Sequence>
    inline typename result_of::as_deque<Sequence const>::type
    as_deque(Sequence const& seq)
    {
        typedef typename result_of::as_deque<Sequence const>::gen gen;
        return gen::call(fusion::begin(seq));
    }
}}

#endif
