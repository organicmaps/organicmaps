/*=============================================================================
    Copyright (c) 2001-2006 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef FUSION_MULTIPLE_VIEW_05052005_0335
#define FUSION_MULTIPLE_VIEW_05052005_0335

#include <boost/mpl/int.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/next.hpp>
#include <boost/fusion/support/detail/access.hpp>
#include <boost/fusion/support/sequence_base.hpp>
#include <boost/fusion/support/iterator_base.hpp>
#include <boost/fusion/support/detail/as_fusion_element.hpp>

namespace boost { namespace fusion
{
    struct multiple_view_tag;
    struct forward_traversal_tag;
    struct fusion_sequence_tag;

    template<typename Size, typename T>
    struct multiple_view
      : sequence_base<multiple_view<Size, T> >
    {
        typedef multiple_view_tag fusion_tag;
        typedef fusion_sequence_tag tag; // this gets picked up by MPL
        typedef forward_traversal_tag category;
        typedef mpl::true_ is_view;
        typedef mpl::int_<Size::value> size;
        typedef T value_type;

        multiple_view()
          : val()
        {}

        explicit multiple_view(typename detail::call_param<T>::type val)
          : val(val)
        {}

        value_type val;
    };

    template<typename Size, typename T>
    inline multiple_view<Size, typename detail::as_fusion_element<T>::type>
    make_multiple_view(T const& v)
    {
        return multiple_view<Size, typename detail::as_fusion_element<T>::type>(v);
    }

    struct multiple_view_iterator_tag;
    struct forward_traversal_tag;

    template<typename Index, typename MultipleView>
    struct multiple_view_iterator
      : iterator_base<multiple_view_iterator<Index, MultipleView> >
    {
        typedef multiple_view_iterator_tag fusion_tag;
        typedef forward_traversal_tag category;
        typedef typename MultipleView::value_type value_type;
        typedef MultipleView multiple_view_type;
        typedef Index index;

        explicit multiple_view_iterator(multiple_view_type const &view_)
          : view(view_)
        {}

        multiple_view_type view;
    };

    namespace extension
    {
        template <typename Tag>
        struct next_impl;

        template <>
        struct next_impl<multiple_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply 
            {
                typedef multiple_view_iterator<
                    typename mpl::next<typename Iterator::index>::type
                  , typename Iterator::multiple_view_type
                > type;
    
                static type
                call(Iterator const &where)
                {
                    return type(where.view);
                }
            };
        };

        template <typename Tag>
        struct end_impl;

        template <>
        struct end_impl<multiple_view_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef multiple_view_iterator<
                    typename Sequence::size
                  , Sequence
                > type;
    
                static type
                call(Sequence &seq)
                {
                    return type(seq);
                }
            };
        };

        template <typename Tag>
        struct deref_impl;

        template <>
        struct deref_impl<multiple_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply
            {
                typedef typename Iterator::value_type type;
    
                static type
                call(Iterator const& i)
                {
                    return i.view.val;
                }
            };
        };

        template <typename Tag>
        struct begin_impl;

        template <>
        struct begin_impl<multiple_view_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef multiple_view_iterator<
                    mpl::int_<0>
                  , Sequence
                > type;
    
                static type
                call(Sequence &seq)
                {
                    return type(seq);
                }
            };
        };

        template <typename Tag>
        struct value_of_impl;

        template <>
        struct value_of_impl<multiple_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply
            {
                typedef typename Iterator::multiple_view_type multiple_view_type;
                typedef typename multiple_view_type::value_type type;
            };
        };
    }
}}

#endif


