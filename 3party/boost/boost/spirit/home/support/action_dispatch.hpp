/*=============================================================================
    Copyright (c) 2001-2011 Joel de Guzman
    Copyright (c) 2001-2011 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ACTION_DISPATCH_APRIL_18_2008_0720AM)
#define BOOST_SPIRIT_ACTION_DISPATCH_APRIL_18_2008_0720AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/phoenix/core/actor.hpp>
#include <boost/spirit/home/support/attributes.hpp>

namespace boost { namespace spirit { namespace traits
{
    template <typename Component>
    struct action_dispatch
    {
        // general handler for everything not explicitly specialized below
        template <typename F, typename Attribute, typename Context>
        bool operator()(F const& f, Attribute& attr, Context& context)
        {
            bool pass = true;
            f(attr, context, pass);
            return pass;
        }

        // handler for phoenix actors

        // If the component this action has to be invoked for is a tuple, we
        // wrap any non-fusion tuple into a fusion tuple (done by pass_attribute)
        // and pass through any fusion tuple.
        template <typename Eval, typename Attribute, typename Context>
        bool operator()(phoenix::actor<Eval> const& f
          , Attribute& attr, Context& context)
        {
            bool pass = true;
            typename pass_attribute<Component, Attribute>::type attr_wrap(attr);
            f(attr_wrap, context, pass);
            return pass;
        }

        // specializations for plain function pointers taking different number of
        // arguments
        template <typename RT, typename A0, typename A1, typename A2
          , typename Attribute, typename Context>
        bool operator()(RT(*f)(A0, A1, A2), Attribute& attr, Context& context)
        {
            bool pass = true;
            f(attr, context, pass);
            return pass;
        }

        template <typename RT, typename A0, typename A1
          , typename Attribute, typename Context>
        bool operator()(RT(*f)(A0, A1), Attribute& attr, Context& context)
        {
            f(attr, context);
            return true;
        }

        template <typename RT, typename A0, typename Attribute, typename Context>
        bool operator()(RT(*f)(A0), Attribute& attr, Context&)
        {
            f(attr);
            return true;
        }

        template <typename RT, typename Attribute, typename Context>
        bool operator()(RT(*f)(), Attribute&, Context&)
        {
            f();
            return true;
        }
    };
}}}

#endif
