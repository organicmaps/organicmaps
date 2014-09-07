// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle


#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_COMPARE_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_COMPARE_TURNS_HPP

#include <cstddef>
#include <functional>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>


namespace boost { namespace geometry
{

namespace detail { namespace turns
{

// TURNS SORTING AND SEARCHING

// sort turns by G1 - source_index == 0 by:
// seg_id -> fraction -> other_id -> operation
template
<
    typename IdLess = std::less<int>,
    int N = 0, int U = 1, int I = 2, int B = 3, int C = 4, int O = 0,
    std::size_t OpId = 0
>
struct less_seg_fraction_other_op
{
    BOOST_STATIC_ASSERT(OpId < 2);

    template <typename Op>
    static inline int order_op(Op const& op)
    {
        switch (op.operation)
        {
        case detail::overlay::operation_none : return N;
        case detail::overlay::operation_union : return U;
        case detail::overlay::operation_intersection : return I;
        case detail::overlay::operation_blocked : return B;
        case detail::overlay::operation_continue : return C;
        case detail::overlay::operation_opposite : return O;
        }
        return -1;
    }

    template <typename Op>
    static inline bool use_operation(Op const& left, Op const& right)
    {
        return order_op(left) < order_op(right);
    }

    template <typename Op>
    static inline bool use_other_id(Op const& left, Op const& right)
    {
        if ( left.other_id.multi_index != right.other_id.multi_index )
        {
            return left.other_id.multi_index < right.other_id.multi_index;
        }
        if ( left.other_id.ring_index != right.other_id.ring_index )
        {
            return left.other_id.ring_index != right.other_id.ring_index;
        }
        if ( left.other_id.segment_index != right.other_id.segment_index )
        {
            return IdLess()(left.other_id.segment_index,
                            right.other_id.segment_index);
        }
        return use_operation(left, right);
    }

    template <typename Op>
    static inline bool use_fraction(Op const& left, Op const& right)
    {
        return left.fraction < right.fraction
           || ( geometry::math::equals(left.fraction, right.fraction) 
                && use_other_id(left, right)
              );
    }

    template <typename Turn>
    inline bool operator()(Turn const& left, Turn const& right) const
    {
        segment_identifier const& sl = left.operations[OpId].seg_id;
        segment_identifier const& sr = right.operations[OpId].seg_id;

        return sl < sr || ( sl == sr && use_fraction(left.operations[OpId], right.operations[OpId]) );
    }
};





}} // namespace detail::turns

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_TURNS_COMPARE_TURNS_HPP
