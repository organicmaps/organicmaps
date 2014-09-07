//-----------------------------------------------------------------------------
// boost variant/detail/move.hpp header file
// See http://www.boost.org for updates, documentation, and revision history.
//-----------------------------------------------------------------------------
//
//  Copyright (c) 2002-2003 Eric Friedman
//  Copyright (c) 2002 by Andrei Alexandrescu
//  Copyright (c) 2013 Antony Polukhin
//
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  This file derivative of MoJO. Much thanks to Andrei for his initial work.
//  See <http://www.cuj.com/experts/2102/alexandr.htm> for information on MOJO.
//  Re-issued here under the Boost Software License, with permission of the original
//  author (Andrei Alexandrescu).


#ifndef BOOST_VARIANT_DETAIL_MOVE_HPP
#define BOOST_VARIANT_DETAIL_MOVE_HPP

#include <iterator> // for iterator_traits
#include <new> // for placement new

#include "boost/config.hpp"
#include "boost/detail/workaround.hpp"
#include "boost/move/move.hpp"

namespace boost { namespace detail { namespace variant {

using boost::move;

//////////////////////////////////////////////////////////////////////////
// function template move_swap
//
// Swaps using Koenig lookup but falls back to move-swap for primitive
// types and on non-conforming compilers.
//

namespace move_swap_fallback {

template <typename T1, typename T2>
inline void swap(T1& lhs, T2& rhs)
{
    T1 tmp( boost::detail::variant::move(lhs) );
    lhs = boost::detail::variant::move(rhs);
    rhs = boost::detail::variant::move(tmp);
}

} // namespace move_swap_fallback

template <typename T>
inline void move_swap(T& lhs, T& rhs)
{
#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
    move_swap_fallback::swap(lhs, rhs);
#else
    using move_swap_fallback::swap;
    swap(lhs, rhs);
#endif
}

}}} // namespace boost::detail::variant

#endif // BOOST_VARIANT_DETAIL_MOVE_HPP



