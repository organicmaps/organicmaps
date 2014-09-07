// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_DEBUG_PRINT_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_DEBUG_PRINT_TURNS_HPP

#ifdef BOOST_GEOMETRY_TEST_DEBUG
#include <iostream>

#include <boost/geometry/io/dsv/write.hpp>
#include <boost/geometry/algorithms/detail/overlay/debug_turn_info.hpp>
#endif


namespace boost { namespace geometry
{

namespace detail { namespace is_valid
{

#ifdef BOOST_GEOMETRY_TEST_DEBUG
template <typename TurnIterator>
inline void debug_print_turns(TurnIterator first, TurnIterator beyond)
{
    std::cout << "turns:";
    for (TurnIterator tit = first; tit != beyond; ++tit)
    {
        std::cout << " ["
                  << geometry::method_char(tit->method)
                  << ","
                  << geometry::operation_char(tit->operations[0].operation)
                  << "/"
                  << geometry::operation_char(tit->operations[1].operation)
                  << " {"
                  << tit->operations[0].seg_id.multi_index
                  << ", "
                  << tit->operations[0].other_id.multi_index
                  << "} {"
                  << tit->operations[0].seg_id.ring_index
                  << ", "
                  << tit->operations[0].other_id.ring_index
                  << "} "
                  << geometry::dsv(tit->point)
                  << "]";
    }
    std::cout << std::endl << std::endl;
}
#else
template <typename TurnIterator>
inline void debug_print_turns(TurnIterator, TurnIterator)
{}
#endif // BOOST_GEOMETRY_TEST_DEBUG

}} // namespace detail::is_valid

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_IS_VALID_DEBUG_PRINT_TURNS_HPP
