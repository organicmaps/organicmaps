/*=============================================================================
    Copyright (c) 2001-2010 Hartmut Kaiser
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#if !defined(BOOST_SPIRIT_ATTR_JUL_23_2008_0956AM)
#define BOOST_SPIRIT_ATTR_JUL_23_2008_0956AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/mpl/bool.hpp>
#include <boost/spirit/home/qi/domain.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/detail/assign_to.hpp>
#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/type_traits/add_reference.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_const.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <typename A0>       // enables attr()
    struct use_terminal<
            qi::domain, terminal_ex<tag::attr, fusion::vector1<A0> > > 
      : mpl::true_ {};

    template <>                  // enables *lazy* attr()
    struct use_lazy_terminal<qi::domain, tag::attr, 1> 
      : mpl::true_ {};

}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::attr;

    template <typename Value>
    struct attr_parser : primitive_parser<attr_parser<Value> >
    {
        template <typename Context, typename Iterator>
        struct attribute : remove_const<Value> {};

        attr_parser(typename add_reference<Value>::type value)
          : value_(value) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& /*first*/, Iterator const& /*last*/
          , Context& /*context*/, Skipper const& /*skipper*/
          , Attribute& attr) const
        {
            spirit::traits::assign_to(value_, attr);
            return true;        // never consume any input, succeed always
        }

        template <typename Context>
        info what(Context& /*context*/) const
        {
            return info("attr");
        }

        Value value_;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        attr_parser& operator= (attr_parser const&);
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Modifiers, typename A0>
    struct make_primitive<
        terminal_ex<tag::attr, fusion::vector1<A0> >
      , Modifiers>
    {
        typedef typename add_const<A0>::type const_value;
        typedef attr_parser<const_value> result_type;

        template <typename Terminal>
        result_type operator()(Terminal const& term, unused_type) const
        {
            return result_type(fusion::at_c<0>(term.args));
        }
    };
}}}

#endif


