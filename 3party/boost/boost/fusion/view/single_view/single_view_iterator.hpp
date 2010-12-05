/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(FUSION_SINGLE_VIEW_ITERATOR_05052005_0340)
#define FUSION_SINGLE_VIEW_ITERATOR_05052005_0340

#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/support/iterator_base.hpp>
#include <boost/fusion/view/single_view/detail/deref_impl.hpp>
#include <boost/fusion/view/single_view/detail/next_impl.hpp>
#include <boost/fusion/view/single_view/detail/value_of_impl.hpp>
#include <boost/config.hpp>

#if defined (BOOST_MSVC)
#  pragma warning(push)
#  pragma warning (disable: 4512) // assignment operator could not be generated.
#endif

namespace boost { namespace fusion
{
    struct single_view_iterator_tag;
    struct forward_traversal_tag;

    template <typename SingleView>
    struct single_view_iterator_end
        : iterator_base<single_view_iterator_end<SingleView> >
    {
        typedef single_view_iterator_tag fusion_tag;
        typedef forward_traversal_tag category;
    };

    template <typename SingleView>
    struct single_view_iterator
        : iterator_base<single_view_iterator<SingleView> >
    {
        typedef single_view_iterator_tag fusion_tag;
        typedef forward_traversal_tag category;
        typedef typename SingleView::value_type value_type;
        typedef SingleView single_view_type;

        explicit single_view_iterator(single_view_type const& view)
            : val(view.val) {}

        value_type val;
    };
}}

#if defined (BOOST_MSVC)
#  pragma warning(pop)
#endif

#endif


