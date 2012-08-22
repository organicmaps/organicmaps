/*=============================================================================
    Copyright (c) 2001-2006 Joel de Guzman
    Copyright (c) 2008 Eric Niebler

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef BOOST_PROTO_DETAIL_FUSION_REVERSE_EAH_01_22_2008
#define BOOST_PROTO_DETAIL_FUSION_REVERSE_EAH_01_22_2008

#include <boost/spirit/fusion/detail/access.hpp>
#include <boost/spirit/fusion/iterator/as_fusion_iterator.hpp>
#include <boost/spirit/fusion/iterator/detail/iterator_base.hpp>
#include <boost/spirit/fusion/sequence/detail/sequence_base.hpp>
#include <boost/spirit/fusion/iterator/next.hpp>
#include <boost/spirit/fusion/iterator/prior.hpp>
#include <boost/spirit/fusion/iterator/deref.hpp>
#include <boost/spirit/fusion/iterator/value_of.hpp>
#include <boost/spirit/fusion/sequence/begin.hpp>
#include <boost/spirit/fusion/sequence/end.hpp>

namespace boost { namespace fusion
{
    struct reverse_view_tag;
    struct reverse_view_iterator_tag;

    template <typename First>
    struct reverse_view_iterator
        : iterator_base<reverse_view_iterator<First> >
    {
        typedef as_fusion_iterator<First> converter;
        typedef typename converter::type first_type;
        typedef reverse_view_iterator_tag tag;

        reverse_view_iterator(First const& first)
            : first(converter::convert(first)) {}

        first_type first;
    };

    template <typename Sequence>
    struct reverse_view : sequence_base<reverse_view<Sequence> >
    {
        typedef as_fusion_sequence<Sequence> seq_converter;
        typedef typename seq_converter::type seq;

        typedef reverse_view_tag tag;
        typedef typename meta::begin<seq>::type first_type;
        typedef typename meta::end<seq>::type last_type;

        reverse_view(Sequence& seq)
          : first(fusion::begin(seq))
          , last(fusion::end(seq))
        {}

        first_type first;
        last_type last;
    };

    namespace meta
    {
        template <>
        struct deref_impl<reverse_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply
            {
                typedef typename
                    meta::deref<
                        typename meta::prior<
                            typename Iterator::first_type
                        >::type
                    >::type
                type;

                static type
                call(Iterator const& i)
                {
                    return *fusion::prior(i.first);
                }
            };
        };

        template <>
        struct prior_impl<reverse_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply
            {
                typedef typename Iterator::first_type first_type;
                typedef typename next_impl<typename first_type::tag>::
                    template apply<first_type>
                wrapped;

                typedef reverse_view_iterator<typename wrapped::type> type;

                static type
                call(Iterator const& i)
                {
                    return type(wrapped::call(i.first));
                }
            };
        };

        template <>
        struct next_impl<reverse_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply
            {
                typedef typename Iterator::first_type first_type;
                typedef typename prior_impl<typename first_type::tag>::
                    template apply<first_type>
                wrapped;

                typedef reverse_view_iterator<typename wrapped::type> type;

                static type
                call(Iterator const& i)
                {
                    return type(wrapped::call(i.first));
                }
            };
        };

        template <>
        struct value_impl<reverse_view_iterator_tag>
        {
            template <typename Iterator>
            struct apply
            {
                typedef typename
                    meta::value_of<
                        typename meta::prior<
                            typename Iterator::first_type
                        >::type
                    >::type
                type;
            };
        };

        template <>
        struct begin_impl<reverse_view_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef reverse_view_iterator<typename Sequence::last_type> type;

                static type
                call(Sequence const& s)
                {
                    return type(s.last);
                }
            };
        };

        template <>
        struct end_impl<reverse_view_tag>
        {
            template <typename Sequence>
            struct apply
            {
                typedef reverse_view_iterator<typename Sequence::first_type> type;

                static type
                call(Sequence const& s)
                {
                    return type(s.first);
                }
            };
        };

        template <typename Sequence>
        struct reverse
        {
            typedef reverse_view<Sequence> type;
        };
    }

    template <typename Sequence>
    inline reverse_view<Sequence const>
    reverse(Sequence const& view)
    {
        return reverse_view<Sequence const>(view);
    }
}}

#endif
