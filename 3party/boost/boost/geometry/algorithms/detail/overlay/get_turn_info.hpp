// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_HPP


#include <boost/assert.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/strategies/intersection.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>

#include <boost/geometry/geometries/segment.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


struct base_turn_handler
{
    // Returns true if both sides are opposite
    static inline bool opposite(int side1, int side2)
    {
        // We cannot state side1 == -side2, because 0 == -0
        // So either side1*side2==-1 or side1==-side2 && side1 != 0
        return side1 * side2 == -1;
    }

    // Same side of a segment (not being 0)
    static inline bool same(int side1, int side2)
    {
        return side1 * side2 == 1;
    }

    // Both continue
    template <typename TurnInfo>
    static inline void both(TurnInfo& ti, operation_type const op)
    {
        ti.operations[0].operation = op;
        ti.operations[1].operation = op;
    }

    // If condition, first union/second intersection, else vice versa
    template <typename TurnInfo>
    static inline void ui_else_iu(bool condition, TurnInfo& ti)
    {
        ti.operations[0].operation = condition
                    ? operation_union : operation_intersection;
        ti.operations[1].operation = condition
                    ? operation_intersection : operation_union;
    }

    // If condition, both union, else both intersection
    template <typename TurnInfo>
    static inline void uu_else_ii(bool condition, TurnInfo& ti)
    {
        both(ti, condition ? operation_union : operation_intersection);
    }
};


template
<
    typename TurnInfo,
    typename SideStrategy
>
struct touch_interior : public base_turn_handler
{
    // Index: 0, P is the interior, Q is touching and vice versa
    template
    <
        int Index,
        typename Point1,
        typename Point2,
        typename IntersectionInfo,
        typename DirInfo
    >
    static inline void apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                TurnInfo& ti,
                IntersectionInfo const& intersection_info,
                DirInfo const& dir_info)
    {
        ti.method = method_touch_interior;
        geometry::convert(intersection_info.intersections[0], ti.point);

        // Both segments of q touch segment p somewhere in its interior
        // 1) We know: if q comes from LEFT or RIGHT
        // (i.e. dir_info.sides.get<Index,0>() == 1 or -1)
        // 2) Important is: if q_k goes to LEFT, RIGHT, COLLINEAR
        //    and, if LEFT/COLL, if it is lying LEFT or RIGHT w.r.t. q_i

        static int const index_p = Index;
        static int const index_q = 1 - Index;

        int const side_qi_p = dir_info.sides.template get<index_q, 0>();
        int const side_qk_p = SideStrategy::apply(pi, pj, qk);

        if (side_qi_p == -side_qk_p)
        {
            // Q crosses P from left->right or from right->left (test "ML1")
            // Union: folow P (left->right) or Q (right->left)
            // Intersection: other turn
            int index = side_qk_p == -1 ? index_p : index_q;
            ti.operations[index].operation = operation_union;
            ti.operations[1 - index].operation = operation_intersection;
            return;
        }

        int const side_qk_q = SideStrategy::apply(qi, qj, qk);

        if (side_qi_p == -1 && side_qk_p == -1 && side_qk_q == 1)
        {
            // Q turns left on the right side of P (test "MR3")
            // Both directions for "intersection"
            both(ti, operation_intersection);
        }
        else if (side_qi_p == 1 && side_qk_p == 1 && side_qk_q == -1)
        {
            // Q turns right on the left side of P (test "ML3")
            // Union: take both operation
            // Intersection: skip
            both(ti, operation_union);
        }
        else if (side_qi_p == side_qk_p && side_qi_p == side_qk_q)
        {
            // Q turns left on the left side of P (test "ML2")
            // or Q turns right on the right side of P (test "MR2")
            // Union: take left turn (Q if Q turns left, P if Q turns right)
            // Intersection: other turn
            int index = side_qk_q == 1 ? index_q : index_p;
            ti.operations[index].operation = operation_union;
            ti.operations[1 - index].operation = operation_intersection;
        }
        else if (side_qk_p == 0)
        {
            // Q intersects on interior of P and continues collinearly
            if (side_qk_q == side_qi_p)
            {
                // Collinearly in the same direction
                // (Q comes from left of P and turns left,
                //  OR Q comes from right of P and turns right)
                // Omit intersection point.
                // Union: just continue
                // Intersection: just continue
                both(ti, operation_continue);
            }
            else
            {
                // Opposite direction, which is never travelled.
                // If Q turns left, P continues for intersection
                // If Q turns right, P continues for union
                ti.operations[Index].operation = side_qk_q == 1
                    ? operation_intersection
                    : operation_union;
                ti.operations[1 - Index].operation = operation_blocked;
            }
        }
        else
        {
            // Should not occur!
            ti.method = method_error;
        }
    }
};


template
<
    typename TurnInfo,
    typename SideStrategy
>
struct touch : public base_turn_handler
{
    static inline bool between(int side1, int side2, int turn)
    {
        return side1 == side2 && ! opposite(side1, turn);
    }

    /*static inline void block_second(bool block, TurnInfo& ti)
    {
        if (block)
        {
            ti.operations[1].operation = operation_blocked;
        }
    }*/


    template
    <
        typename Point1,
        typename Point2,
        typename IntersectionInfo,
        typename DirInfo
    >
    static inline void apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                TurnInfo& ti,
                IntersectionInfo const& intersection_info,
                DirInfo const& dir_info)
    {
        ti.method = method_touch;
        geometry::convert(intersection_info.intersections[0], ti.point);

        int const side_qi_p1 = dir_info.sides.template get<1, 0>();
        int const side_qk_p1 = SideStrategy::apply(pi, pj, qk);


        // If Qi and Qk are both at same side of Pi-Pj,
        // or collinear (so: not opposite sides)
        if (! opposite(side_qi_p1, side_qk_p1))
        {
            int const side_pk_q2 = SideStrategy::apply(qj, qk, pk);
            int const side_pk_p  = SideStrategy::apply(pi, pj, pk);
            int const side_qk_q  = SideStrategy::apply(qi, qj, qk);
            bool const q_turns_left = side_qk_q == 1;
            bool const block_q = side_qk_p1 == 0
                        && ! same(side_qi_p1, side_qk_q);

            // If Pk at same side as Qi/Qk
            // (the "or" is for collinear case)
            // or Q is fully collinear && P turns not to left
            if (side_pk_p == side_qi_p1
                || side_pk_p == side_qk_p1
                || (side_qi_p1 == 0 && side_qk_p1 == 0 && side_pk_p != -1)
                )
            {
                // Collinear -> lines join, continue
                // (#BRL2)
                if (side_pk_q2 == 0 && ! block_q)
                {
                    both(ti, operation_continue);
                    return;
                }

                int const side_pk_q1 = SideStrategy::apply(qi, qj, pk);


                // Collinear opposite case -> block P
                // (#BRL4, #BLR8)
                if (side_pk_q1 == 0)
                {
                    ti.operations[0].operation = operation_blocked;
                    // Q turns right -> union (both independant),
                    // Q turns left -> intersection
                    ti.operations[1].operation = block_q ? operation_blocked
                        : q_turns_left ? operation_intersection
                        : operation_union;
                    return;
                }

                // Pk between Qi and Qk
                // (#BRL3, #BRL7)
                if (between(side_pk_q1, side_pk_q2, side_qk_q))
                {
                    ui_else_iu(q_turns_left, ti);
                    if (block_q)
                    {
                        ti.operations[1].operation = operation_blocked;
                    }
                    //block_second(block_q, ti);
                    return;
                }

                // Pk between Qk and P, so left of Qk (if Q turns right) and vv
                // (#BRL1)
                if (side_pk_q2 == -side_qk_q)
                {
                    ui_else_iu(! q_turns_left, ti);
                    return;
                }

                //
                // (#BRL5, #BRL9)
                if (side_pk_q1 == -side_qk_q)
                {
                    uu_else_ii(! q_turns_left, ti);
                    if (block_q)
                    {
                        ti.operations[1].operation = operation_blocked;
                    }
                    //block_second(block_q, ti);
                    return;
                }
            }
            else
            {
                // Pk at other side than Qi/Pk
                int const side_qk_q = SideStrategy::apply(qi, qj, qk);
                bool const q_turns_left = side_qk_q == 1;

                ti.operations[0].operation = q_turns_left
                            ? operation_intersection
                            : operation_union;
                ti.operations[1].operation = block_q
                            ? operation_blocked
                            : side_qi_p1 == 1 || side_qk_p1 == 1
                            ? operation_union
                            : operation_intersection;

                return;
            }
        }
        else
        {
            // From left to right or from right to left
            int const side_pk_p  = SideStrategy::apply(pi, pj, pk);
            bool const right_to_left = side_qk_p1 == 1;

            // If p turns into direction of qi (1,2)
            if (side_pk_p == side_qi_p1)
            {
                int const side_pk_q1 = SideStrategy::apply(qi, qj, pk);

                // Collinear opposite case -> block P
                if (side_pk_q1 == 0)
                {
                    ti.operations[0].operation = operation_blocked;
                    ti.operations[1].operation = right_to_left
                                ? operation_union : operation_intersection;
                    return;
                }

                if (side_pk_q1 == side_qk_p1)
                {
                    uu_else_ii(right_to_left, ti);
                    return;
                }
            }

            // If p turns into direction of qk (4,5)
            if (side_pk_p == side_qk_p1)
            {
                int const side_pk_q2 = SideStrategy::apply(qj, qk, pk);

                // Collinear case -> lines join, continue
                if (side_pk_q2 == 0)
                {
                    both(ti, operation_continue);
                    return;
                }
                if (side_pk_q2 == side_qk_p1)
                {
                    ui_else_iu(right_to_left, ti);
                    return;
                }
            }
            // otherwise (3)
            ui_else_iu(! right_to_left, ti);
            return;
        }

#ifdef BOOST_GEOMETRY_DEBUG_GET_TURNS
        // Normally a robustness issue.
        // TODO: more research if still occuring
        std::cout << "Not yet handled" << std::endl
            << "pi " << get<0>(pi) << " , " << get<1>(pi)
            << " pj " << get<0>(pj) << " , " << get<1>(pj)
            << " pk " << get<0>(pk) << " , " << get<1>(pk)
            << std::endl
            << "qi " << get<0>(qi) << " , " << get<1>(qi)
            << " qj " << get<0>(qj) << " , " << get<1>(qj)
            << " qk " << get<0>(qk) << " , " << get<1>(qk)
            << std::endl;
#endif

    }
};


template
<
    typename TurnInfo,
    typename SideStrategy
>
struct equal : public base_turn_handler
{
    template
    <
        typename Point1,
        typename Point2,
        typename IntersectionInfo,
        typename DirInfo
    >
    static inline void apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                TurnInfo& ti,
                IntersectionInfo const& intersection_info,
                DirInfo const& dir_info)
    {
        ti.method = method_equal;
        // Copy the SECOND intersection point
        geometry::convert(intersection_info.intersections[1], ti.point);

        int const side_pk_q2  = SideStrategy::apply(qj, qk, pk);
        int const side_pk_p = SideStrategy::apply(pi, pj, pk);
        int const side_qk_p = SideStrategy::apply(pi, pj, qk);

        // If pk is collinear with qj-qk, they continue collinearly.
        // This can be on either side of p1 (== q1), or collinear
        // The second condition checks if they do not continue
        // oppositely
        if (side_pk_q2 == 0 && side_pk_p == side_qk_p)
        {
            both(ti, operation_continue);
            return;
        }


        // If they turn to same side (not opposite sides)
        if (! opposite(side_pk_p, side_qk_p))
        {
            int const side_pk_q2 = SideStrategy::apply(qj, qk, pk);

            // If pk is left of q2 or collinear: p: union, q: intersection
            ui_else_iu(side_pk_q2 != -1, ti);
        }
        else
        {
            // They turn opposite sides. If p turns left (or collinear),
            // p: union, q: intersection
            ui_else_iu(side_pk_p != -1, ti);
        }
    }
};


template
<
    typename TurnInfo,
    typename SideStrategy
>
struct collinear : public base_turn_handler
{
    /*
        arrival P   pk//p1  qk//q1   product*  case    result
         1           1                1        CLL1    ui
        -1                   1       -1        CLL2    iu
         1           1                1        CLR1    ui
        -1                  -1        1        CLR2    ui

         1          -1               -1        CRL1    iu
        -1                   1       -1        CRL2    iu
         1          -1               -1        CRR1    iu
        -1                  -1        1        CRR2    ui

         1           0                0        CC1     cc
        -1                   0        0        CC2     cc

         *product = arrival * (pk//p1 or qk//q1)

         Stated otherwise:
         - if P arrives: look at turn P
         - if Q arrives: look at turn Q
         - if P arrives and P turns left: union for P
         - if P arrives and P turns right: intersection for P
         - if Q arrives and Q turns left: union for Q (=intersection for P)
         - if Q arrives and Q turns right: intersection for Q (=union for P)
    */
    template
    <
        typename Point1,
        typename Point2,
        typename IntersectionInfo,
        typename DirInfo
    >
    static inline void apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                TurnInfo& ti,
                IntersectionInfo const& intersection_info,
                DirInfo const& dir_info)
    {
        ti.method = method_collinear;
        geometry::convert(intersection_info.intersections[1], ti.point);

        int const arrival = dir_info.arrival[0];
        // Should not be 0, this is checked before
        BOOST_ASSERT(arrival != 0);

        // If p arrives, use p, else use q
        int const side_p_or_q = arrival == 1
            ? SideStrategy::apply(pi, pj, pk)
            : SideStrategy::apply(qi, qj, qk)
            ;

        // See comments above,
        // resulting in a strange sort of mathematic rule here:
        // The arrival-info multiplied by the relevant side
        // delivers a consistent result.

        int const product = arrival * side_p_or_q;

        if(product == 0)
        {
            both(ti, operation_continue);
        }
        else
        {
            ui_else_iu(product == 1, ti);
        }
    }
};

template
<
    typename TurnInfo,
    typename SideStrategy,
    typename AssignPolicy
>
struct collinear_opposite : public base_turn_handler
{
private :
    /*
        arrival P  arrival Q  pk//p1   qk//q1  case  result2  result
        --------------------------------------------------------------
         1          1          1       -1      CLO1    ix      xu
         1          1          1        0      CLO2    ix      (xx)
         1          1          1        1      CLO3    ix      xi

         1          1          0       -1      CCO1    (xx)    xu
         1          1          0        0      CCO2    (xx)    (xx)
         1          1          0        1      CCO3    (xx)    xi

         1          1         -1       -1      CRO1    ux      xu
         1          1         -1        0      CRO2    ux      (xx)
         1          1         -1        1      CRO3    ux      xi

        -1          1                  -1      CXO1    xu
        -1          1                   0      CXO2    (xx)
        -1          1                   1      CXO3    xi

         1         -1          1               CXO1    ix
         1         -1          0               CXO2    (xx)
         1         -1         -1               CXO3    ux
    */

    template
    <
        int Index,
        typename Point,
        typename IntersectionInfo
    >
    static inline bool set_tp(Point const& ri, Point const& rj, Point const& rk,
                TurnInfo& tp, IntersectionInfo const& intersection_info)
    {
        int const side_rk_r = SideStrategy::apply(ri, rj, rk);
        switch(side_rk_r)
        {

            case 1 :
                // Turning left on opposite collinear: intersection
                tp.operations[Index].operation = operation_intersection;
                break;
            case -1 :
                // Turning right on opposite collinear: union
                tp.operations[Index].operation = operation_union;
                break;
            case 0 :
                // No turn on opposite collinear: block, do not traverse
                // But this "xx" is ignored here, it is useless to include
                // two operation blocked, so the whole point does not need
                // to be generated.
                // So return false to indicate nothing is to be done.
                return false;
        }

        // The other direction is always blocked when collinear opposite
        tp.operations[1 - Index].operation = operation_blocked;

        // If P arrives within Q, set info on P (which is done above, index=0),
        // this turn-info belongs to the second intersection point, index=1
        // (see e.g. figure CLO1)
        geometry::convert(intersection_info.intersections[1 - Index], tp.point);
        return true;
    }

public:
    template
    <
        typename Point1,
        typename Point2,
        typename OutputIterator,
        typename IntersectionInfo,
        typename DirInfo
    >
    static inline void apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,

                // Opposite collinear can deliver 2 intersection points,
                TurnInfo const& tp_model,
                OutputIterator& out,

                IntersectionInfo const& intersection_info,
                DirInfo const& dir_info)
    {
        /*
        std::cout << "arrivals: "
            << dir_info.arrival[0]
            << "/" << dir_info.arrival[1]
            << std::endl;
        */

        TurnInfo tp = tp_model;

        tp.method = method_collinear;

        // If P arrives within Q, there is a turn dependant on P
        if (dir_info.arrival[0] == 1
            && set_tp<0>(pi, pj, pk, tp, intersection_info))
        {
            AssignPolicy::apply(tp, pi, qi);
            *out++ = tp;
        }

        // If Q arrives within P, there is a turn dependant on Q
        if (dir_info.arrival[1] == 1
            && set_tp<1>(qi, qj, qk, tp, intersection_info))
        {
            AssignPolicy::apply(tp, pi, qi);
            *out++ = tp;
        }
    }
};


template
<
    typename TurnInfo,
    typename SideStrategy
>
struct crosses : public base_turn_handler
{
    template
    <
        typename Point1,
        typename Point2,
        typename IntersectionInfo,
        typename DirInfo
    >
    static inline void apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                TurnInfo& ti,
                IntersectionInfo const& intersection_info,
                DirInfo const& dir_info)
    {
        ti.method = method_crosses;
        geometry::convert(intersection_info.intersections[0], ti.point);

        // In all casees:
        // If Q crosses P from left to right
        // Union: take P
        // Intersection: take Q
        // Otherwise: vice versa
        int const side_qi_p1 = dir_info.sides.template get<1, 0>();
        int const index = side_qi_p1 == 1 ? 0 : 1;
        ti.operations[index].operation = operation_union;
        ti.operations[1 - index].operation = operation_intersection;
    }
};


/*!
\brief Policy doing nothing
\details get_turn_info can have an optional policy to get/assign some
    extra information. By default it does not, and this class
    is that default.
 */
struct assign_null_policy
{
    template <typename Point1, typename Point2, typename Info>
    static inline void apply(Info& info, Point1 const& p1, Point2 const& p2)
    {}

};


/*!
    \brief Turn information: intersection point, method, and turn information
    \details Information necessary for traversal phase (a phase
        of the overlay process). The information is gathered during the
        get_turns (segment intersection) phase.
    \tparam Point1 point type of first segment
    \tparam Point2 point type of second segment
    \tparam TurnInfo type of class getting intersection and turn info
    \tparam AssignPolicy policy to assign extra info,
        e.g. to calculate distance from segment's first points
        to intersection points
 */
template
<
    typename Point1,
    typename Point2,
    typename TurnInfo,
    typename AssignPolicy = assign_null_policy
>
struct get_turn_info
{
    typedef strategy_intersection
        <
            typename cs_tag<typename TurnInfo::point_type>::type,
            Point1,
            Point2,
            typename TurnInfo::point_type
        > si;

    typedef typename si::segment_intersection_strategy_type strategy;



    template <typename OutputIterator>
    static inline OutputIterator apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                TurnInfo const& tp_model,
                OutputIterator out)
    {
        typedef model::referring_segment<Point1 const> segment_type1;
        typedef model::referring_segment<Point1 const> segment_type2;
        segment_type1 p1(pi, pj), p2(pj, pk);
        segment_type2 q1(qi, qj), q2(qj, qk);

        typename strategy::return_type result = strategy::apply(p1, q1);

        char const method = result.template get<1>().how;

        // Copy, to copy possibly extended fields
        TurnInfo tp = tp_model;


        // Select method and apply
        switch(method)
        {
            case 'a' :
            case 'f' :
            case 's' :
            case 'd' :
                break;

            case 'm' :
            {
                typedef touch_interior
                    <
                        TurnInfo,
                        typename si::side_strategy_type
                    > policy;

                // If Q (1) arrives (1)
                if (result.template get<1>().arrival[1] == 1)
                {
                    policy::template apply<0>(pi, pj, pk, qi, qj, qk,
                                tp, result.template get<0>(), result.template get<1>());
                }
                else
                {
                    // Swap p/q
                    policy::template apply<1>(qi, qj, qk, pi, pj, pk,
                                tp, result.template get<0>(), result.template get<1>());
                }
                AssignPolicy::apply(tp, pi, qi);
                *out++ = tp;
            }
            break;
            case 'i' :
            {
                typedef crosses
                    <
                        TurnInfo,
                        typename si::side_strategy_type
                    > policy;

                policy::apply(pi, pj, pk, qi, qj, qk,
                    tp, result.template get<0>(), result.template get<1>());
                AssignPolicy::apply(tp, pi, qi);
                *out++ = tp;
            }
            break;
            case 't' :
            {
                // Both touch (both arrive there)
                typedef touch
                    <
                        TurnInfo,
                        typename si::side_strategy_type
                    > policy;

                policy::apply(pi, pj, pk, qi, qj, qk,
                    tp, result.template get<0>(), result.template get<1>());
                AssignPolicy::apply(tp, pi, qi);
                *out++ = tp;
            }
            break;
            case 'e':
            {
                if (! result.template get<1>().opposite)
                {
                    // Both equal
                    // or collinear-and-ending at intersection point
                    typedef equal
                        <
                            TurnInfo,
                            typename si::side_strategy_type
                        > policy;

                    policy::apply(pi, pj, pk, qi, qj, qk,
                        tp, result.template get<0>(), result.template get<1>());
                    AssignPolicy::apply(tp, pi, qi);
                    *out++ = tp;
                }
                // If they ARE opposite, don't do anything.
            }
            break;
            case 'c' :
            {
                // Collinear
                if (! result.template get<1>().opposite)
                {

                    if (result.template get<1>().arrival[0] == 0)
                    {
                        // Collinear, but similar thus handled as equal
                        equal
                            <
                                TurnInfo,
                                typename si::side_strategy_type
                            >::apply(pi, pj, pk, qi, qj, qk,
                                tp, result.template get<0>(), result.template get<1>());

                        // override assigned method
                        tp.method = method_collinear;
                    }
                    else
                    {
                        collinear
                            <
                                TurnInfo,
                                typename si::side_strategy_type
                            >::apply(pi, pj, pk, qi, qj, qk,
                                tp, result.template get<0>(), result.template get<1>());
                    }

                    AssignPolicy::apply(tp, pi, qi);
                    *out++ = tp;
                }
                else
                {
                    collinear_opposite
                        <
                            TurnInfo,
                            typename si::side_strategy_type,
                            AssignPolicy
                        >::apply(pi, pj, pk, qi, qj, qk,
                            tp, out, result.template get<0>(), result.template get<1>());
                }
            }
            break;
            case '0' :
               // degenerate points
               break;
            default :
#ifdef BOOST_GEOMETRY_DEBUG_GET_TURNS
                std::cout << "get_turns, nyi: " << method << std::endl;
#endif
                break;
        }

        return out;
    }
};


}} // namespace detail::overlay
#endif //DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_HPP
