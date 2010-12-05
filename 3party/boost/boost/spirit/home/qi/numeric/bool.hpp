/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(SPIRIT_QI_BOOL_SEP_29_2009_0709AM)
#define SPIRIT_QI_BOOL_SEP_29_2009_0709AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/numeric/bool_policies.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/detail/workaround.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<qi::domain, tag::bool_> // enables bool_
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::true_> // enables true_
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::false_> // enables false_
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::bool_;
    using spirit::bool__type;
    using spirit::true_;
    using spirit::true__type;
    using spirit::false_;
    using spirit::false__type;

    namespace detail
    {
        template <typename T, typename Policies>
        struct bool_impl
        {
            template <typename Iterator, typename Attribute>
            static bool parse(Iterator& first, Iterator const& last
              , Attribute& attr, Policies const& p, bool allow_true = true
              , bool disallow_false = false) 
            {
                if (first == last)
                    return false;

#if BOOST_WORKAROUND(BOOST_MSVC, BOOST_TESTED_AT(1600))
                p; // suppresses warning: C4100: 'p' : unreferenced formal parameter
#endif
                return (allow_true && p.parse_true(first, last, attr)) ||
                       (!disallow_false && p.parse_false(first, last, attr));
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    // This actual boolean parser
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T = bool
      , typename Policies = bool_policies<T> >
    struct bool_parser_impl
      : primitive_parser<bool_parser_impl<T, Policies> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef T type;
        };

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr) const
        {
            qi::skip_over(first, last, skipper);
            return detail::bool_impl<T, Policies>::
                parse(first, last, attr, Policies());
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("boolean");
        }
    };

    template <
        typename T = bool
      , typename Policies = bool_policies<T> >
    struct bool_parser_literal_impl
      : primitive_parser<bool_parser_literal_impl<T, Policies> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef T type;
        };

        bool_parser_literal_impl(typename add_const<T>::type n)
          : n_(n)
        {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr) const
        {
            qi::skip_over(first, last, skipper);
            return detail::bool_impl<T, Policies>::
                parse(first, last, attr, Policies(), n_, n_);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("boolean");
        }

        T n_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // bool_parser is the class that the user can instantiate directly
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T
      , typename Policies = bool_policies<T> >
    struct bool_parser
      : proto::terminal<bool_parser_impl<T, Policies> >::type
    {};

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::bool_, Modifiers>
    {
        typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;

        typedef typename mpl::if_<
            no_case
          , bool_parser_impl<bool, no_case_bool_policies<> > 
          , bool_parser_impl<> >::type
        result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    namespace detail
    {
        template <typename Modifiers, bool b>
        struct make_literal_bool
        {
            typedef has_modifier<Modifiers, tag::char_code_base<tag::no_case> > no_case;

            typedef typename mpl::if_<
                no_case
              , bool_parser_literal_impl<bool, no_case_bool_policies<> > 
              , bool_parser_literal_impl<> >::type
            result_type;

            result_type operator()(unused_type, unused_type) const
            {
                return result_type(b);
            }
        };
    }

    template <typename Modifiers>
    struct make_primitive<tag::false_, Modifiers>
      : detail::make_literal_bool<Modifiers, false>
    {};

    template <typename Modifiers>
    struct make_primitive<tag::true_, Modifiers>
      : detail::make_literal_bool<Modifiers, true>
    {};

}}}

#endif
