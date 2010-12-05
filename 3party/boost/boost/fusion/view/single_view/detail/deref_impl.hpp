/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_DEREF_IMPL_05052005_0258)
#define FUSION_DEREF_IMPL_05052005_0258

#include <boost/fusion/support/detail/access.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_const.hpp>

namespace boost { namespace fusion
{
    struct single_view_iterator_tag;

    namespace extension
    {
        template <typename Tag>
        struct deref_impl;

        template <>
        struct deref_impl<single_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply
            {
                typedef typename Iterator::value_type type;
    
                static type
                call(Iterator const& i)
                {
                    return i.val;
                }
            };
        };
    }
}}

#endif


