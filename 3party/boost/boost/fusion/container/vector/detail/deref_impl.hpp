/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_DEREF_IMPL_05042005_1037)
#define FUSION_DEREF_IMPL_05042005_1037

#include <boost/mpl/at.hpp>
#include <boost/fusion/support/detail/access.hpp>
#include <boost/type_traits/is_const.hpp>

namespace boost { namespace fusion
{
    struct vector_iterator_tag;

    namespace extension
    {
        template <typename Tag>
        struct deref_impl;

        template <>
        struct deref_impl<vector_iterator_tag>
        {
            template <typename Iterator>
            struct apply 
            {
                typedef typename Iterator::vector vector;
                typedef typename Iterator::index index;
                typedef typename mpl::at<
                    typename vector::types, index> 
                element;
                
                typedef typename
                    mpl::eval_if<
                        is_const<vector>
                      , fusion::detail::cref_result<element>
                      , fusion::detail::ref_result<element>
                    >::type
                type;

                static type
                call(Iterator const& i)
                {
                    return i.vec.at_impl(index());
                }
            };
        };
    }
}}

#endif
