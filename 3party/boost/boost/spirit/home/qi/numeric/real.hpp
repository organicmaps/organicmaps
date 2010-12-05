/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_REAL_APRIL_18_2006_0850AM)
#define BOOST_SPIRIT_REAL_APRIL_18_2006_0850AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/numeric/real_policies.hpp>
#include <boost/spirit/home/qi/numeric/numeric_utils.hpp>
#include <boost/spirit/home/qi/numeric/detail/real_impl.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<qi::domain, tag::float_> // enables float_
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::double_> // enables double_
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::long_double> // enables long_double
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::float_;
    using spirit::float__type;
    using spirit::double_;
    using spirit::double__type;
    using spirit::long_double;
    using spirit::long_double_type;

    ///////////////////////////////////////////////////////////////////////////
    // This is the actual real number parser
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T = double,
        typename RealPolicies = real_policies<T>
    >
    struct real_parser_impl
      : primitive_parser<real_parser_impl<T, RealPolicies> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef T type;
        };

        template <typename Iterator, typename Context, typename Skipper>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , T& attr) const
        {
            qi::skip_over(first, last, skipper);
            return detail::real_impl<T, RealPolicies>::
                parse(first, last, attr, RealPolicies());
        }

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr_) const
        {
            // this case is called when Attribute is not T
            T attr;
            if (parse(first, last, context, skipper, attr))
            {
                traits::assign_to(attr, attr_);
                return true;
            }
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("real-number");
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // This one is the class that the user can instantiate directly
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T,
        typename RealPolicies = real_policies<T>
    >
    struct real_parser
      : proto::terminal<real_parser_impl<T, RealPolicies> >::type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct make_real
    {
        typedef real_parser_impl<T, real_policies<T> > result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    template <typename Modifiers>
    struct make_primitive<tag::float_, Modifiers> : make_real<float> {};

    template <typename Modifiers>
    struct make_primitive<tag::double_, Modifiers> : make_real<double> {};

    template <typename Modifiers>
    struct make_primitive<tag::long_double, Modifiers> : make_real<long double> {};
}}}

#endif
