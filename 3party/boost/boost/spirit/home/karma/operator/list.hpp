//  Copyright (c) 2001-2010 Hartmut Kaiser
//  Copyright (c) 2001-2010 Joel de Guzman
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(SPIRIT_KARMA_LIST_MAY_01_2007_0229PM)
#define SPIRIT_KARMA_LIST_MAY_01_2007_0229PM

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
#include <boost/spirit/home/karma/detail/attributes.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_operator<karma::domain, proto::tag::modulus> // enables g % d
      : mpl::true_ {};
}}

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace spirit { namespace karma
{
    template <typename Left, typename Right, typename Strict, typename Derived>
    struct base_list : binary_generator<Derived>
    {
    private:
        // iterate over the given container until its exhausted or the embedded
        // (left) generator succeeds
        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Iterator, typename Attribute>
        bool generate_left(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Iterator& it, Iterator& end, Attribute const&) const
        {
            if (Strict::value) {
                if (!traits::compare(it, end))
                    return left.generate(sink, ctx, d, traits::deref(it));
            }
            else {
                // Failing subject generators are just skipped. This allows to 
                // selectively generate items in the provided attribute.
                while (!traits::compare(it, end))
                {
                    if (left.generate(sink, ctx, d, traits::deref(it)))
                        return true;
                    traits::next(it);
                }
            }
            return false;
        }

        template <
            typename OutputIterator, typename Context, typename Delimiter
          , typename Iterator>
        bool generate_left(OutputIterator& sink, Context& ctx
          , Delimiter const& d, Iterator&, Iterator&, unused_type) const
        {
            // There is no way to distinguish a failed generator from a 
            // generator to be skipped. We assume the user takes responsibility
            // for ending the loop if no attribute is specified.
            return left.generate(sink, ctx, d, unused);
        }

    public:
        typedef Left left_type;
        typedef Right right_type;

        typedef mpl::int_<
            left_type::properties::value 
          | right_type::properties::value 
          | generator_properties::buffering 
          | generator_properties::counting
        > properties;

        // Build a std::vector from the LHS's attribute. Note
        // that build_std_vector may return unused_type if the
        // subject's attribute is an unused_type.
        template <typename Context, typename Iterator>
        struct attribute
          : traits::build_std_vector<
                typename traits::attribute_of<Left, Context, Iterator>::type>
        {};

        base_list(Left const& left, Right const& right)
          : left(left), right(right) 
        {}

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

            if (generate_left(sink, ctx, d, it, end, attr))
            {
                for (traits::next(it); !traits::compare(it, end); traits::next(it))
                {
                    // wrap the given output iterator as generate_left might fail
                    detail::enable_buffering<OutputIterator> buffering(sink);
                    {
                        detail::disable_counting<OutputIterator> nocounting(sink);

                        if (!right.generate(sink, ctx, d, unused))
                            return false;     // shouldn't happen

                        if (!generate_left(sink, ctx, d, it, end, attr))
                            break;            // return true as one item succeeded
                    }
                    buffering.buffer_copy();
                }
                return true;
            }
            return false;
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

    template <typename Left, typename Right>
    struct list 
      : base_list<Left, Right, mpl::false_, list<Left, Right> >
    {
        typedef base_list<Left, Right, mpl::false_, list> base_list_;

        list(Left const& left, Right const& right)
          : base_list_(left, right) {}
    };

    template <typename Left, typename Right>
    struct strict_list 
      : base_list<Left, Right, mpl::true_, strict_list<Left, Right> >
    {
        typedef base_list<Left, Right, mpl::true_, strict_list> base_list_;

        strict_list (Left const& left, Right const& right)
          : base_list_(left, right) {}
    };

    ///////////////////////////////////////////////////////////////////////////
    // Generator generators: make_xxx function (objects)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename Subject, bool strict_mode = false>
        struct make_list 
          : make_binary_composite<Subject, list>
        {};

        template <typename Subject>
        struct make_list<Subject, true> 
          : make_binary_composite<Subject, strict_list>
        {};
    }

    template <typename Subject, typename Modifiers>
    struct make_composite<proto::tag::modulus, Subject, Modifiers>
      : detail::make_list<Subject, detail::get_stricttag<Modifiers>::value>
    {};
}}}

namespace boost { namespace spirit { namespace traits
{
    template <typename Left, typename Right>
    struct has_semantic_action<karma::list<Left, Right> >
      : binary_has_semantic_action<Left, Right> {};

    template <typename Left, typename Right>
    struct has_semantic_action<karma::strict_list<Left, Right> >
      : binary_has_semantic_action<Left, Right> {};
}}}

#endif
