//  Copyright (c) 2001-2011 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_SPIRIT_KARMA_GENERATE_DEC_01_2009_0734PM)
#define BOOST_SPIRIT_KARMA_GENERATE_DEC_01_2009_0734PM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/karma/detail/generate.hpp>

namespace boost { namespace spirit { namespace karma
{
    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Expr>
    inline bool
    generate(
        OutputIterator& sink
      , Expr const& expr)
    {
        return detail::generate_impl<Expr>::call(sink, expr);
    }

    template <typename OutputIterator, typename Expr>
    inline bool
    generate(
        OutputIterator const& sink_
      , Expr const& expr)
    {
        OutputIterator sink = sink_;
        return generate(sink, expr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Expr, typename Attr>
    inline bool
    generate(
        OutputIterator& sink_
      , Expr const& expr
      , Attr const& attr)
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);

        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Expr>::type
        > properties;

        // wrap user supplied iterator into our own output iterator
        detail::output_iterator<OutputIterator
          , mpl::int_<properties::value> > sink(sink_);
        return compile<karma::domain>(expr).generate(sink, unused, unused, attr);
    }

    template <typename OutputIterator, typename Properties, typename Expr
      , typename Attr>
    inline bool
    generate(
        detail::output_iterator<OutputIterator, Properties>& sink
      , Expr const& expr
      , Attr const& attr)
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then the expression (expr) is not a valid spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);

        return compile<karma::domain>(expr).generate(sink, unused, unused, attr);
    }

    template <typename OutputIterator, typename Expr, typename Attr>
    inline bool
    generate(
        OutputIterator const& sink_
      , Expr const& expr
      , Attr const& attr)
    {
        OutputIterator sink = sink_;
        return generate(sink, expr, attr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Expr, typename Delimiter>
    inline bool
    generate_delimited(
        OutputIterator& sink
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit = 
            delimit_flag::dont_predelimit)
    {
        return detail::generate_delimited_impl<Expr>::call(
            sink, expr, delimiter, pre_delimit);
    }

    template <typename OutputIterator, typename Expr, typename Delimiter>
    inline bool
    generate_delimited(
        OutputIterator const& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit = 
            delimit_flag::dont_predelimit)
    {
        OutputIterator sink = sink_;
        return generate_delimited(sink, expr, delimiter, pre_delimit);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Properties, typename Expr
      , typename Delimiter, typename Attribute>
    inline bool
    generate_delimited(
        detail::output_iterator<OutputIterator, Properties>& sink
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , Attribute const& attr)
    {
        // Report invalid expression error as early as possible.
        // If you got an error_invalid_expression error message here,
        // then either the expression (expr) or skipper is not a valid
        // spirit karma expression.
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Expr);
        BOOST_SPIRIT_ASSERT_MATCH(karma::domain, Delimiter);

        typename result_of::compile<karma::domain, Delimiter>::type const 
            delimiter_ = compile<karma::domain>(delimiter);

        if (pre_delimit == delimit_flag::predelimit &&
            !karma::delimit_out(sink, delimiter_))
        {
            return false;
        }
        return compile<karma::domain>(expr).
            generate(sink, unused, delimiter_, attr);
    }

    template <typename OutputIterator, typename Expr, typename Delimiter
      , typename Attribute>
    inline bool
    generate_delimited(
        OutputIterator& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , Attribute const& attr)
    {
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Expr>::type
        > properties;
        typedef traits::properties_of<
            typename result_of::compile<karma::domain, Delimiter>::type
        > delimiter_properties;

        // wrap user supplied iterator into our own output iterator
        detail::output_iterator<OutputIterator
          , mpl::int_<properties::value | delimiter_properties::value>
        > sink(sink_);
        return karma::generate_delimited(sink, expr, delimiter, pre_delimit, attr);
    }

    template <typename OutputIterator, typename Expr, typename Delimiter
      , typename Attribute>
    inline bool
    generate_delimited(
        OutputIterator const& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , BOOST_SCOPED_ENUM(delimit_flag) pre_delimit
      , Attribute const& attr)
    {
        OutputIterator sink = sink_;
        return generate_delimited(sink, expr, delimiter, pre_delimit, attr);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename OutputIterator, typename Expr, typename Delimiter
      , typename Attribute>
    inline bool
    generate_delimited(
        OutputIterator& sink
      , Expr const& expr
      , Delimiter const& delimiter
      , Attribute const& attr)
    {
        return karma::generate_delimited(sink, expr, delimiter
          , delimit_flag::dont_predelimit, attr);
    }

    template <typename OutputIterator, typename Expr, typename Delimiter
      , typename Attribute>
    inline bool
    generate_delimited(
        OutputIterator const& sink_
      , Expr const& expr
      , Delimiter const& delimiter
      , Attribute const& attr)
    {
        OutputIterator sink = sink_;
        return karma::generate_delimited(sink, expr, delimiter
          , delimit_flag::dont_predelimit, attr);
    }

}}}

#endif

