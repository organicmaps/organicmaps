/*=============================================================================
    Copyright (c) 2001-2010 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(SPIRIT_REPEAT_NOVEMBER_14_2008_1148AM)
#define SPIRIT_REPEAT_NOVEMBER_14_2008_1148AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/qi/meta_compiler.hpp>
#include <boost/spirit/home/qi/parser.hpp>
#include <boost/spirit/home/qi/auxiliary/lazy.hpp>
#include <boost/spirit/home/qi/operator/kleene.hpp>
#include <boost/spirit/home/support/container.hpp>
#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/qi/detail/attributes.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/foreach.hpp>
#include <vector>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<qi::domain, tag::repeat>   // enables repeat[p]
      : mpl::true_ {};

    template <typename T>
    struct use_directive<qi::domain
      , terminal_ex<tag::repeat                     // enables repeat(exact)[p]
        , fusion::vector1<T> >
    > : mpl::true_ {};

    template <typename T>
    struct use_directive<qi::domain
      , terminal_ex<tag::repeat                     // enables repeat(min, max)[p]
        , fusion::vector2<T, T> >
    > : mpl::true_ {};

    template <typename T>
    struct use_directive<qi::domain
      , terminal_ex<tag::repeat                     // enables repeat(min, inf)[p]
        , fusion::vector2<T, inf_type> >
    > : mpl::true_ {};

    template <>                                     // enables *lazy* repeat(exact)[p]
    struct use_lazy_directive<
        qi::domain
      , tag::repeat
      , 1 // arity
    > : mpl::true_ {};

    template <>                                     // enables *lazy* repeat(min, max)[p]
    struct use_lazy_directive<                      // and repeat(min, inf)[p]
        qi::domain
      , tag::repeat
      , 2 // arity
    > : mpl::true_ {};
}}

namespace boost { namespace spirit { namespace qi
{
    using spirit::repeat;
    using spirit::repeat_type;
    using spirit::inf;
    using spirit::inf_type;

    template <typename T>
    struct exact_iterator // handles repeat(exact)[p]
    {
        exact_iterator(T const exact)
          : exact(exact) {}

        typedef T type;
        T start() const { return 0; }
        bool got_max(T i) const { return i >= exact; }
        bool got_min(T i) const { return i >= exact; }

        T const exact;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        exact_iterator& operator= (exact_iterator const&);
    };

    template <typename T>
    struct finite_iterator // handles repeat(min, max)[p]
    {
        finite_iterator(T const min, T const max)
          : min BOOST_PREVENT_MACRO_SUBSTITUTION (min)
          , max BOOST_PREVENT_MACRO_SUBSTITUTION (max) {}

        typedef T type;
        T start() const { return 0; }
        bool got_max(T i) const { return i >= max; }
        bool got_min(T i) const { return i >= min; }

        T const min;
        T const max;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        finite_iterator& operator= (finite_iterator const&);
    };

    template <typename T>
    struct infinite_iterator // handles repeat(min, inf)[p]
    {
        infinite_iterator(T const min)
          : min BOOST_PREVENT_MACRO_SUBSTITUTION (min) {}

        typedef T type;
        T start() const { return 0; }
        bool got_max(T /*i*/) const { return false; }
        bool got_min(T i) const { return i >= min; }

        T const min;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        infinite_iterator& operator= (infinite_iterator const&);
    };

    template <typename Subject, typename LoopIter>
    struct repeat_parser : unary_parser<repeat_parser<Subject, LoopIter> >
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
                    typename traits::attribute_of<
                        Subject, Context, Iterator>::type
                >::type
            type;
        };

        repeat_parser(Subject const& subject, LoopIter const& iter)
          : subject(subject), iter(iter) {}

        template <typename Iterator, typename Context
          , typename Skipper, typename ValueType, typename Attribute
          , typename LoopVar>
        bool parse_minimal(Iterator &first, Iterator const& last
          , Context& context, Skipper const& skipper
          , Attribute& attr, ValueType& val, LoopVar& i) const
        {
            // this scope allows save and required_attr to be reclaimed 
            // immediately after we're done with the required minimum 
            // iteration.
            Iterator save = first;
            std::vector<ValueType> required_attr;
            for (; !iter.got_min(i); ++i)
            {
                if (!subject.parse(save, last, context, skipper, val) ||
                    !traits::push_back(required_attr, val))
                {
                    return false;
                }

                first = save;
                traits::clear(val);
            }

            // if we got the required number of items, these are copied 
            // over (appended) to the 'real' attribute
            BOOST_FOREACH(ValueType const& v, required_attr)
            {
                traits::push_back(attr, v);
            }
            return true;
        }

        template <typename Iterator, typename Context
          , typename Skipper, typename LoopVar>
        bool parse_minimal(Iterator &first, Iterator const& last
          , Context& context, Skipper const& skipper
          , unused_type, unused_type, LoopVar& i) const
        {
            // this scope allows save and required_attr to be reclaimed 
            // immediately after we're done with the required minimum 
            // iteration.
            Iterator save = first;
            for (; !iter.got_min(i); ++i)
            {
                if (!subject.parse(save, last, context, skipper, unused))
                {
                    return false;
                }
                first = save;
            }
            return true;
        }

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
            typename LoopIter::type i = iter.start();

            // parse the minimum required
            if (!iter.got_min(i) &&
                !parse_minimal(first, last, context, skipper, attr, val, i))
            {
                return false;
            }

            // parse some more up to the maximum specified
            Iterator save = first;
            for (; !iter.got_max(i); ++i)
            {
                if (!subject.parse(save, last, context, skipper, val) ||
                    !traits::push_back(attr, val))
                {
                    break;
                }
                first = save;
                traits::clear(val);
            }
            return true;
        }

        template <typename Context>
        info what(Context& context) const
        {
            return info("repeat", subject.what(context));
        }

        Subject subject;
        LoopIter iter;

    private:
        // silence MSVC warning C4512: assignment operator could not be generated
        repeat_parser& operator= (repeat_parser const&);
    };

    ///////////////////////////////////////////////////////////////////////////
    // Parser generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    template <typename Subject, typename Modifiers>
    struct make_directive<tag::repeat, Subject, Modifiers>
    {
        typedef kleene<Subject> result_type;
        result_type operator()(unused_type, Subject const& subject, unused_type) const
        {
            return result_type(subject);
        }
    };

    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::repeat, fusion::vector1<T> >, Subject, Modifiers>
    {
        typedef exact_iterator<T> iterator_type;
        typedef repeat_parser<Subject, iterator_type> result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args));
        }
    };

    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::repeat, fusion::vector2<T, T> >, Subject, Modifiers>
    {
        typedef finite_iterator<T> iterator_type;
        typedef repeat_parser<Subject, iterator_type> result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject,
                iterator_type(
                    fusion::at_c<0>(term.args)
                  , fusion::at_c<1>(term.args)
                )
            );
        }
    };

    template <typename T, typename Subject, typename Modifiers>
    struct make_directive<
        terminal_ex<tag::repeat
        , fusion::vector2<T, inf_type> >, Subject, Modifiers>
    {
        typedef infinite_iterator<T> iterator_type;
        typedef repeat_parser<Subject, iterator_type> result_type;

        template <typename Terminal>
        result_type operator()(
            Terminal const& term, Subject const& subject, unused_type) const
        {
            return result_type(subject, fusion::at_c<0>(term.args));
        }
    };
}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Subject, typename LoopIter>
    struct has_semantic_action<qi::repeat_parser<Subject, LoopIter> >
      : unary_has_semantic_action<Subject> {};
}}}

#endif
