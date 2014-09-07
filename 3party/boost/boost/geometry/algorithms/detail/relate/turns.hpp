// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013, 2014.
// Modifications copyright (c) 2013-2014 Oracle and/or its affiliates.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_TURNS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_TURNS_HPP

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/algorithms/detail/overlay/do_reverse.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>

#include <boost/type_traits/is_base_of.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace relate { namespace turns {

template <bool IncludeDegenerate = false>
struct assign_policy
    : overlay::assign_null_policy
{
    static bool const include_degenerate = IncludeDegenerate;
};

// GET_TURNS

template <typename Geometry1,
          typename Geometry2,
          typename GetTurnPolicy
            = detail::get_turns::get_turn_info_type<Geometry1, Geometry2, assign_policy<> > >
struct get_turns
{
    typedef typename geometry::point_type<Geometry1>::type point1_type;

    typedef overlay::turn_info
            <
                point1_type,
                typename segment_ratio_type<point1_type, detail::no_rescale_policy>::type,
                typename detail::get_turns::turn_operation_type
                    <
                        Geometry1, Geometry2,
                        typename segment_ratio_type<point1_type, detail::no_rescale_policy>::type
                    >::type
            > turn_info;

    template <typename Turns>
    static inline void apply(Turns & turns,
                             Geometry1 const& geometry1,
                             Geometry2 const& geometry2)
    {
        detail::get_turns::no_interrupt_policy interrupt_policy;

        apply(turns, geometry1, geometry2, interrupt_policy);
    }

    template <typename Turns, typename InterruptPolicy>
    static inline void apply(Turns & turns,
                             Geometry1 const& geometry1,
                             Geometry2 const& geometry2,
                             InterruptPolicy & interrupt_policy)
    {
        static const bool reverse1 = detail::overlay::do_reverse<geometry::point_order<Geometry1>::value>::value;
        static const bool reverse2 = detail::overlay::do_reverse<geometry::point_order<Geometry2>::value>::value;

        dispatch::get_turns
            <
                typename geometry::tag<Geometry1>::type,
                typename geometry::tag<Geometry2>::type,
                Geometry1,
                Geometry2,
                reverse1,
                reverse2,
                GetTurnPolicy
            >::apply(0, geometry1, 1, geometry2,
                     detail::no_rescale_policy(), turns, interrupt_policy);
    }
};

// TURNS SORTING AND SEARCHING

template <int N = 0, int U = 1, int I = 2, int B = 3, int C = 4, int O = 0>
struct op_to_int
{
    template <typename Op>
    inline int operator()(Op const& op) const
    {
        switch(op.operation)
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
};

template <typename OpToInt>
struct less_op_xxx_linear
{
    template <typename Op>
    inline bool operator()(Op const& left, Op const& right)
    {
        static OpToInt op_to_int;
        return op_to_int(left) < op_to_int(right);
    }
};

struct less_op_linear_linear
    : less_op_xxx_linear< op_to_int<0,2,3,1,4,0> >
{};

struct less_op_linear_areal
{
    template <typename Op>
    inline bool operator()(Op const& left, Op const& right)
    {
        static turns::op_to_int<0,2,3,1,4,0> op_to_int_xuic;
        static turns::op_to_int<0,3,2,1,4,0> op_to_int_xiuc;

        if ( left.other_id.multi_index == right.other_id.multi_index )
        {
            if ( left.other_id.ring_index == right.other_id.ring_index )
                return op_to_int_xuic(left) < op_to_int_xuic(right);
            else
                return op_to_int_xiuc(left) < op_to_int_xiuc(right);
        }
        else
        {
            //return op_to_int_xuic(left) < op_to_int_xuic(right);
            return left.other_id.multi_index < right.other_id.multi_index;
        }
    }
};

struct less_op_areal_linear
    : less_op_xxx_linear< op_to_int<0,1,0,0,2,0> >
{};

struct less_op_areal_areal
{
    template <typename Op>
    inline bool operator()(Op const& left, Op const& right)
    {
        static op_to_int<0, 1, 2, 3, 4, 0> op_to_int_uixc;
        static op_to_int<0, 2, 1, 3, 4, 0> op_to_int_iuxc;

        if ( left.other_id.multi_index == right.other_id.multi_index )
        {
            if ( left.other_id.ring_index == right.other_id.ring_index )
            {
                return op_to_int_uixc(left) < op_to_int_uixc(right);
            }
            else
            {
                if ( left.other_id.ring_index == -1 )
                {
                    if ( left.operation == overlay::operation_union )
                        return false;
                    else if ( left.operation == overlay::operation_intersection )
                        return true;
                }
                else if ( right.other_id.ring_index == -1 )
                {
                    if ( right.operation == overlay::operation_union )
                        return true;
                    else if ( right.operation == overlay::operation_intersection )
                        return false;
                }
                
                return op_to_int_iuxc(left) < op_to_int_iuxc(right);
            }
        }
        else
        {
            return op_to_int_uixc(left) < op_to_int_uixc(right);
        }
    }
};

// sort turns by G1 - source_index == 0 by:
// seg_id -> distance -> operation
template <std::size_t OpId = 0,
          typename LessOp = less_op_xxx_linear< op_to_int<> > >
struct less
{
    BOOST_STATIC_ASSERT(OpId < 2);

    template <typename Op> static inline
    bool use_fraction(Op const& left, Op const& right)
    {
        static LessOp less_op;

        if ( left.fraction == right.fraction )
            return less_op(left, right);
        else
            return left.fraction < right.fraction;
    }

    template <typename Turn>
    inline bool operator()(Turn const& left, Turn const& right) const
    {
        segment_identifier const& sl = left.operations[OpId].seg_id;
        segment_identifier const& sr = right.operations[OpId].seg_id;

        return sl < sr || ( sl == sr && use_fraction(left.operations[OpId], right.operations[OpId]) );
    }
};

}}} // namespace detail::relate::turns
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_RELATE_TURNS_HPP
