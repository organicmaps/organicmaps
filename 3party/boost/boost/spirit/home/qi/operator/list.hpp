/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_LIST_MARCH_24_2007_1031AM)
#define SPIRIT_LIST_MARCH_24_2007_1031AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <vector>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<qi::domain, proto::tag::modulus> // enables p % d
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    template <typename Left, typename Right>
    struct list : binary_parser<list<Left, Right> >
    {
        typedef Left left_type;
        typedef Right right_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            // Build a std::vector from the LHS's attribute. Note
            // that build_std_vector may return unused_type if the
            // subject's attribute is an unused_type.
            typedef typename
                traits::build_std_vector<
                    typename traits::
                        attribute_of<Left, Context, Iterator>::type
                >::type
            type;
        };

        list(Left const& left, Right const& right)
          : left(left), right(right) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename Attribute>
        bool parse(Iterator& first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr) const
        {
            // create a local value if Attribute is not unused_type
            typedef typename traits::container_value<Attribute>::type 
                value_type;
            value_type val = value_type();

            Iterator save = first;
            if (!left.parse(save, last, context, skipper, val) ||
                !traits::push_back(attr, val))
            {
                return false;
            }
            first = save;

            while (right.parse(save, last, context, skipper, unused)
             && (traits::clear(val), true)
             && left.parse(save, last, context, skipper, val))
            {
                if (!traits::push_back(attr, val))
                    break;
                first = save;
            }
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("list",
                std::make_pair(left.what(context), right.what(context)));
        }

        Left left;
        Right right;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::modulus, Elements, Modifiers>
      : make_binary_composite<Elements, list>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Left, typename Right>
    struct has_semantic_action<qi::list<Left, Right> >
      : binary_has_semantic_action<Left, Right> {};
}}}

#endif
