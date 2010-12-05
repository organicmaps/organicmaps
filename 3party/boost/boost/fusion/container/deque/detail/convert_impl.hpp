/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2005-2006 Dan Marsden

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_CONVERT_IMPL_20061213_2207)
#define FUSION_CONVERT_IMPL_20061213_2207

#include <boost/fusion/container/deque/detail/as_deque.hpp>
#include <boost/fusion/container/deque/deque.hpp>
#include <boost/fusion/sequence/intrinsic/begin.hpp>
#include <boost/fusion/sequence/intrinsic/size.hpp>

namespace boost { namespace fusion
{
    struct deque_tag;

    namespace extension
    {
        template <typename T>
        struct convert_impl;

        template <>
        struct convert_impl<deque_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef typename detail::as_deque<result_of::size<Sequence>::value> gen;
                typedef typename gen::
                    template apply<typename result_of::begin<Sequence>::type>::type
                type;

                static type call(Sequence& seq)
                {
                    return gen::call(fusion::begin(seq));
                }
            };
        };
    }
}}

#endif
