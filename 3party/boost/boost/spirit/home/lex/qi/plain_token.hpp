//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_LEX_PLAIN_TOKEN_NOV_11_2007_0451PM)
#define BOOST_SPIRIT_LEX_PLAIN_TOKEN_NOV_11_2007_0451PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/qi/skip_over.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_enum.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////

    // enables token
    template <>
    struct use_terminal<qi::domain, tag::token>
      : mpl::true_ {};

    // enables token(id)
    template <typename A0>
    struct use_terminal<qi::domain
      , terminal_ex<tag::token, fusion::vector1<A0> >
    > : mpl::or_<is_integral<A0>, is_enum<A0> > {};

    // enables *lazy* token(id)
    template <>
    struct use_lazy_terminal<
        qi::domain, tag::token, 1
    > : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::token;

    ///////////////////////////////////////////////////////////////////////////
    template <typename TokenId>
    struct plain_token 
      : primitive_parser<plain_token<TokenId> >
    {
        template <typename Context, typename Iterator>
        struct attribute
        {
            typedef typename Iterator::base_iterator_type iterator_type;
            typedef iterator_range<iterator_type> type;
        };

        plain_token(TokenId const& id)
          : id(id) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& /*context*/, Skipper const& skipper
          , Attribute& attr) const
        {
            qi::skip_over(first, last, skipper);   // always do a pre-skip

            if (first != last) {
                // simply match the token id with the id this component has 
                // been initialized with

                typedef typename 
                    boost::detail::iterator_traits<Iterator>::value_type 
                token_type;
                typedef typename token_type::id_type id_type;

                token_type const& t = *first;
                if (std::size_t(~0) == t.id() || id_type(id) == t.id()) {
                    spirit::traits::assign_to(t, attr);
                    ++first;
                    return true;
                }
            }
            return false;
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("token");
        }

        TokenId id;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers>
    struct make_primitive<tag::token, Modifiers>
    {
        typedef plain_token<std::size_t> result_type;

        result_type operator()(unused_type, unused_type) const
        {
            return result_type(std::size_t(~0));
        }
    };

    template <typename Modifiers, typename TokenId>
    struct make_primitive<terminal_ex<tag::token, fusion::vector1<TokenId> >
      , Modifiers>
    {
        typedef plain_token<TokenId> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };
}}}

#endif
