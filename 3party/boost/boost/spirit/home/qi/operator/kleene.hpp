/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_KLEENE_JANUARY_07_2007_0818AM)
#define SPIRIT_KLEENE_JANUARY_07_2007_0818AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    //[composite_parsers_kleene_enable_
    template <>
    struct use_operator<qi::domain, proto::tag::dereference> // enables *p
      : mpl::true_ {};
    //]
}}

namespace boost { namespace spirit { namespace qi
{

    //[composite_parsers_kleene
    template <typename Subject>
    struct kleene : unary_parser<kleene<Subject> >
    {
        typedef Subject subject_type;

        template <typename Context, typename Iterator>
        struct attribute
        {
            // Build a std::vector from the subject's attribute. Note
            // that build_std_vector may return unused_type if the
            // subject's attribute is an unused_type.
            typedef typename
                traits::build_std_vector<
                    typename traits::
                        attribute_of<Subject, Context, Iterator>::type
                >::type
            type;
        };

        kleene(Subject const& subject)
          : subject(subject) {}

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

            // Repeat while subject parses ok
            Iterator save = first;
            while (subject.parse(save, last, context, skipper, val) &&
                   traits::push_back(attr, val))    // push the parsed value into our attribute
            {
                first = save;
                traits::clear(val);
            }
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("kleene", subject.what(context));
        }

        Subject subject;
    };
    //]

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    //[composite_parsers_kleene_generator
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::dereference, Elements, Modifiers>
      : make_unary_composite<Elements, kleene>
    {};
    //]

//     ///////////////////////////////////////////////////////////////////////////
//     // Define what attributes are compatible with a kleene
//     template <typename Attribute, typename Subject, typename Context, typename Iterator>
//     struct is_attribute_compatible<Attribute, kleene<Subject>, Context, Iterator>
//       : traits::is_container_compatible<qi::domain, Attribute
//               , kleene<Subject>, Context, Iterator>
//     {};
}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Subject>
    struct has_semantic_action<qi::kleene<Subject> >
      : unary_has_semantic_action<Subject> {};
}}}

#endif
