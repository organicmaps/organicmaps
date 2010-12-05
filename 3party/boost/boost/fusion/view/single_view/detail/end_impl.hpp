/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_END_IMPL_05052005_0332)
#define FUSION_END_IMPL_05052005_0332

namespace boost { namespace fusion
{
    struct single_view_tag;

    template <typename T>
    struct single_view_iterator_end;

    namespace extension
    {
        template <typename Tag>
        struct end_impl;

        template <>
        struct end_impl<single_view_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef single_view_iterator_end<Sequence> type;
    
                static type
                call(Sequence&)
                {
                    return type();
                }
            };
        };
    }
}}

#endif


