//  Copyright (c) 2001-2010 Hartmut Kaiser
// 
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(SPIRIT_STRICT_RELAXED_APR_22_2010_0959AM)
#define SPIRIT_STRICT_RELAXED_APR_22_2010_0959AM

#if defined(_MSC_VER)
#pragma once
#endif

#include <boost/spirit/home/support/common_terminals.hpp>
#include <boost/spirit/home/support/modify.hpp>
#include <boost/spirit/home/karma/domain.hpp>
#include <boost/spirit/home/karma/meta_compiler.hpp>

namespace boost { namespace spirit
{
    ///////////////////////////////////////////////////////////////////////////
    // Enablers
    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct use_directive<karma::domain, tag::strict>  // enables strict[]
      : mpl::true_ {};

    template <>
    struct use_directive<karma::domain, tag::relaxed> // enables relaxed[]
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct is_modifier_directive<karma::domain, tag::strict>
      : mpl::true_ {};

    template <>
    struct is_modifier_directive<karma::domain, tag::relaxed>
      : mpl::true_ {};

    ///////////////////////////////////////////////////////////////////////////
    // Don't add tag::strict or tag::relaxed if there is already one of those 
    // in the modifier list
    template <typename Current>
    struct compound_modifier<Current, tag::strict
          , typename enable_if<has_modifier<Current, tag::relaxed> >::type>
      : Current
    {
        compound_modifier()
          : Current() {}

        compound_modifier(Current const& current, tag::strict const&)
          : Current(current) {}
    };

    template <typename Current>
    struct compound_modifier<Current, tag::relaxed
          , typename enable_if<has_modifier<Current, tag::strict> >::type>
      : Current
    {
        compound_modifier()
          : Current() {}

        compound_modifier(Current const& current, tag::relaxed const&)
          : Current(current) {}
    };

    namespace karma
    {
        using boost::spirit::strict;
        using boost::spirit::strict_type;
        using boost::spirit::relaxed;
        using boost::spirit::relaxed_type;
    }
}}

#endif
