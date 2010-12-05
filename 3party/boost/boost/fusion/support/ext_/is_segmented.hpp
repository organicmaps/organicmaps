/*=============================================================================
    Copyright (c) 2006 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_IS_SEGMENTED_03202006_0015)
#define FUSION_IS_SEGMENTED_03202006_0015

#include <boost/fusion/support/tag_of.hpp>

namespace boost { namespace fusion 
{
    // Special tags:
    struct sequence_facade_tag;
    struct boost_tuple_tag; // boost::tuples::tuple tag
    struct boost_array_tag; // boost::array tag
    struct mpl_sequence_tag; // mpl sequence tag
    struct std_pair_tag; // std::pair tag
    struct iterator_range_tag;

    namespace extension
    {
        template<typename Tag>
        struct is_segmented_impl
        {
            template<typename Sequence>
            struct apply
              : mpl::false_
            {};
        };

        template<>
        struct is_segmented_impl<iterator_range_tag>;
    }

    namespace traits
    {
        template <typename Sequence>
        struct is_segmented
          : extension::is_segmented_impl<typename traits::tag_of<Sequence>::type>::
                template apply<Sequence>
        {
        };
    }
}}

#endif
