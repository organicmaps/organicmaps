//  Copyright (c) 2001-2010 Hartmut Kaiser
//  Copyright (c) 2001-2010 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(SPIRIT_KARMA_AND_PREDICATE_MAR_22_2009_0412PM)
#define SPIRIT_KARMA_AND_PREDICATE_MAR_22_2009_0412PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::address_of> // enables &g
      : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace karma
{
    template <typename Subject>
    struct and_predicate : unary_generator<and_predicate<Subject> >
    {
        typedef Subject subject_type;
        typedef mpl::int_<
            generator_properties::disabling | subject_type::properties::value
        > properties;

        template <typename Context, typename Iterator>
        struct attribute
          : traits::attribute_of<subject_type, Context, Iterator>
        {};

        and_predicate(Subject const& subject)
          : subject(subject) {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx, Delimiter const& d
          , Attribute const& attr) const
        {
            // inhibits output
            detail::disable_output<OutputIterator> disable(sink);
            return subject.generate(sink, ctx, d, attr);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("and-predicate", subject.what(context));
        }

        Subject subject;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::address_of, Elements, Modifiers>
      : make_unary_composite<Elements, and_predicate> {};

}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Subject>
    struct has_semantic_action<karma::and_predicate<Subject> >
      : unary_has_semantic_action<Subject> {};

}}}

#endif
