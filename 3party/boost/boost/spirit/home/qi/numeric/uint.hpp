/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(SPIRIT_UINT_APR_17_2006_0901AM)
#define SPIRIT_UINT_APR_17_2006_0901AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/numeric/numeric_utils.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/mpl/assert.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_terminal<qi::domain, tag::bin> // enables bin
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::oct> // enables oct
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::hex> // enables hex
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::ushort_> // enables ushort_
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::ulong_> // enables ulong_
      : mpl::true_ {};

    template <>
    struct use_terminal<qi::domain, tag::uint_> // enables uint_
      : mpl::true_ {};

#ifdef BOOST_HAS_LONG_LONG
    template <>
    struct use_terminal<qi::domain, tag::ulong_long> // enables ulong_long
      : mpl::true_ {};
#endif
}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::bin;
    using spirit::bin_type;
    using spirit::oct;
    using spirit::oct_type;
    using spirit::hex;
    using spirit::hex_type;
    using spirit::ushort_;
    using spirit::ushort__type;
    using spirit::ulong_;
    using spirit::ulong__type;
    using spirit::uint_;
    using spirit::uint__type;
#ifdef BOOST_HAS_LONG_LONG
    using spirit::ulong_long;
    using spirit::ulong_long_type;
#endif

    ///////////////////////////////////////////////////////////////////////////
    // This actual unsigned int parser
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T
      , unsigned Radix = 10
      , unsigned MinDigits = 1
      , int MaxDigits = -1>
    struct uint_parser_impl
      : primitive_parser<uint_parser_impl<T, Radix, MinDigits, MaxDigits> >
    {
        // check template parameter 'Radix' for validity
        BOOST_SPIRIT_ASSERT_MSG(
            Radix == 2 || Radix == 8 || Radix == 10 || Radix == 16,
            not_supported_radix, ());

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
            return extract_uint<T, Radix, MinDigits, MaxDigits>
                ::call(first, last, attr);
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("unsigned-integer");
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // uint_parser is the class that the user can instantiate directly
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T
      , unsigned Radix = 10
      , unsigned MinDigits = 1
      , int MaxDigits = -1>
    struct uint_parser
      : proto::terminal<uint_parser_impl<T, Radix, MinDigits, MaxDigits> >::type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <
        typename T
      , unsigned Radix = 10
      , unsigned MinDigits = 1
      , int MaxDigits = -1>
    struct make_uint
    {
        typedef uint_parser_impl<T, Radix, MinDigits, MaxDigits> result_type;
        result_type operator()(unused_type, unused_type) const
        {
            return result_type();
        }
    };

    template <typename Modifiers>
    struct make_primitive<tag::bin, Modifiers>
      : make_uint<unsigned, 2, 1, -1> {};

    template <typename Modifiers>
    struct make_primitive<tag::oct, Modifiers>
      : make_uint<unsigned, 8, 1, -1> {};

    template <typename Modifiers>
    struct make_primitive<tag::hex, Modifiers>
      : make_uint<unsigned, 16, 1, -1> {};

    template <typename Modifiers>
    struct make_primitive<tag::ushort_, Modifiers>
      : make_uint<unsigned short> {};

    template <typename Modifiers>
    struct make_primitive<tag::ulong_, Modifiers>
      : make_uint<unsigned long> {};

    template <typename Modifiers>
    struct make_primitive<tag::uint_, Modifiers>
      : make_uint<unsigned int> {};

#ifdef BOOST_HAS_LONG_LONG
    template <typename Modifiers>
    struct make_primitive<tag::ulong_long, Modifiers>
      : make_uint<boost::ulong_long_type> {};
#endif
}}}

#endif
