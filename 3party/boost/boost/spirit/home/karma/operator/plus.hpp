//  Copyright (c) 2001-2011 Joel de Guzman
//  Copyright (c) 2001-2011 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_POSITIVE_MAR_03_2007_0945PM)
#define BOOST_SPIRIT_KARMA_POSITIVE_MAR_03_2007_0945PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/generator.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>
#include <boost/spirit/home/karma/detail/output_iterator.hpp>
#include <boost/spirit/home/karma/detail/get_stricttag.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/home/support/unused.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/has_semantic_action.hpp>
#include <boost/spirit/home/support/handles_container.hpp>
#include <boost/spirit/home/karma/detail/attributes.hpp>

#include <boost/type_traits/add_const.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::unary_plus> // enables +g
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    template <typename Subject, typename Strict, typename Derived>
    struct base_plus : unary_generator<Derived>
    {
    private:
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate_subject(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute const& attr, bool& result) const
        {
            // Ignore return value, failing subject generators are just 
            // skipped. This allows to selectively generate items in the 
            // provided attribute.
            if (subject.generate(sink, ctx, d, attr)) {
                result = true;
                return true;
            }
            return !Strict::value;
        }

        template <typename OutputIterator, typename Context, typename Delimiter>
        bool generate_subject(OutputIterator& sink, Context& ctx
          , Delimiter const& d, unused_type, bool& result) const
        {
            // There is no way to distinguish a failed generator from a 
            // generator to be skipped. We assume the user takes responsibility
            // for ending the loop if no attribute is specified.
            if (subject.generate(sink, ctx, d, unused)) {
                result = true;
                return true;
            }
            return false;
        }

    public:
        typedef Subject subject_type;
        typedef typename subject_type::properties properties;

        // Build a std::vector from the subjects attribute. Note
        // that build_std_vector may return unused_type if the
        // subject's attribute is an unused_type.
        template <typename Context, typename Iterator>
        struct attribute
          : traits::build_std_vector<
                typename traits::attribute_of<subject_type, Context, Iterator>::type
            >
        {};

        base_plus(Subject const& subject)
          : subject(subject) {}

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Attribute>
        bool generate(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Attribute const& attr) const
        {
            typedef typename traits::container_iterator<
                typename add_const<Attribute>::type
            >::type iterator_type;

            iterator_type it = traits::begin(attr);
            iterator_type end = traits::end(attr);

            // plus fails if the parameter is empty
            if (traits::compare(it, end))
                return false;

            // from now on plus fails if the underlying output fails or overall
            // no subject generators succeeded
            bool result = false;
            for (/**/; detail::sink_is_good(sink) && !traits::compare(it, end); 
                 traits::next(it))
            {
                if (!generate_subject(sink, ctx, d, traits::deref(it), result))
                    break;
            }
            return result && detail::sink_is_good(sink);
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("plus", subject.what(context));
        }

        Subject subject;
    };

    template <typename Subject>
    struct plus 
      : base_plus<Subject, mpl::false_, plus<Subject> >
    {
        typedef base_plus<Subject, mpl::false_, plus> base_plus_;

        plus(Subject const& subject)
          : base_plus_(subject) {}
    };

    template <typename Subject>
    struct strict_plus 
      : base_plus<Subject, mpl::true_, strict_plus<Subject> >
    {
        typedef base_plus<Subject, mpl::true_, strict_plus> base_plus_;

        strict_plus(Subject const& subject)
          : base_plus_(subject) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Elements, bool strict_mode = false>
        struct make_plus 
          : make_unary_composite<Elements, plus>
        {};

        template <typename Elements>
        struct make_plus<Elements, true> 
          : make_unary_composite<Elements, strict_plus>
        {};
    }

    template <typename Elements, typename Modifiers>
    struct make_composite<proto::tag::unary_plus, Elements, Modifiers>
      : detail::make_plus<Elements, detail::get_stricttag<Modifiers>::value>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject>
    struct has_semantic_action<karma::plus<Subject> >
      : unary_has_semantic_action<Subject> {};

    template <typename Subject>
    struct has_semantic_action<karma::strict_plus<Subject> >
      : unary_has_semantic_action<Subject> {};

    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::plus<Subject>, Attribute
      , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};

    template <typename Subject, typename Attribute, typename Context
      , typename Iterator>
    struct handles_container<karma::strict_plus<Subject>, Attribute
      , Context, Iterator>
      : unary_handles_container<Subject, Attribute, Context, Iterator> {};
}}}

#endif
