/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_NEXT_IMPL_05052005_0331)
#define FUSION_NEXT_IMPL_05052005_0331

namespace boost { namespace fusion
{
    struct single_view_iterator_tag;

    template <typename SingleView>
    struct single_view_iterator_end;

    template <typename SingleView>
    struct single_view_iterator;

    namespace extension
    {
        template <typename Tag>
        struct next_impl;

        template <>
        struct next_impl<single_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply 
            {
                typedef single_view_iterator_end<
                    typename Iterator::single_view_type>
                type;
    
                static type
                call(Iterator)
                {
                    return type();
                }
            };
        };
    }
}}

#endif


