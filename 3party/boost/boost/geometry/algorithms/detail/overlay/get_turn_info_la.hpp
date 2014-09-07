// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013, 2014.
// Modifications copyright (c) 2013-2014 Oracle and/or its affiliates.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_LA_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_LA_HPP

#include <boost/geometry/algorithms/detail/overlay/get_turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turn_info_for_endpoint.hpp>

// TEMP, for spikes detector
//#include <boost/geometry/algorithms/detail/overlay/get_turn_info_ll.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay {

template<typename AssignPolicy>
struct get_turn_info_linear_areal
{
    static const bool handle_spikes = true;

    template
    <
        typename Point1,
        typename Point2,
        typename TurnInfo,
        typename RobustPolicy,
        typename OutputIterator
    >
    static inline OutputIterator apply(
                Point1 const& pi, Point1 const& pj, Point1 const& pk,
                Point2 const& qi, Point2 const& qj, Point2 const& qk,
                bool is_p_first, bool is_p_last,
                bool is_q_first, bool is_q_last,
                TurnInfo const& tp_model,
                RobustPolicy const& robust_policy,
                OutputIterator out)
    {
        typedef intersection_info<Point1, Point2, typename TurnInfo::point_type, RobustPolicy>
            inters_info;

        inters_info inters(pi, pj, pk, qi, qj, qk, robust_policy);

        char const method = inters.d_info().how;

        // Copy, to copy possibly extended fields
        TurnInfo tp = tp_model;

        // Select method and apply
        switch(method)
        {
            case 'a' : // collinear, "at"
            case 'f' : // collinear, "from"
            case 's' : // starts from the middle
                get_turn_info_for_endpoint<true, true>(
                    pi, pj, pk, qi, qj, qk,
                    is_p_first, is_p_last, is_q_first, is_q_last,
                    tp_model, inters, method_none, out);
                break;

            case 'd' : // disjoint: never do anything
                break;

            case 'm' :
            {
                if ( get_turn_info_for_endpoint<false, true>(
                        pi, pj, pk, qi, qj, qk,
                        is_p_first, is_p_last, is_q_first, is_q_last,
                        tp_model, inters, method_touch_interior, out) )
                {
                    // do nothing
                }
                else
                {
                    typedef touch_interior
                        <
                            TurnInfo
                        > policy;

                    // If Q (1) arrives (1)
                    if ( inters.d_info().arrival[1] == 1 )
                    {
                        policy::template apply<0>(pi, pj, pk, qi, qj, qk,
                                    tp, inters.i_info(), inters.d_info(),
                                    inters.sides());
                    }
                    else
                    {
                        // Swap p/q
                        side_calculator
                            <
                                typename inters_info::robust_point2_type,
                                typename inters_info::robust_point1_type
                            > swapped_side_calc(inters.rqi(), inters.rqj(), inters.rqk(),
                                                inters.rpi(), inters.rpj(), inters.rpk());
                        policy::template apply<1>(qi, qj, qk, pi, pj, pk,
                                    tp, inters.i_info(), inters.d_info(),
                                    swapped_side_calc);
                    }

                    if ( tp.operations[1].operation == operation_blocked )
                    {
                        tp.operations[0].is_collinear = true;
                    }

                    replace_method_and_operations_tm(tp.method,
                                                     tp.operations[0].operation,
                                                     tp.operations[1].operation);
                    
                    // this function assumes that 'u' must be set for a spike
                    calculate_spike_operation(tp.operations[0].operation,
                                              inters, is_p_last);
                    
                    AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());

                    *out++ = tp;
                }
            }
            break;
            case 'i' :
            {
                crosses<TurnInfo>::apply(pi, pj, pk, qi, qj, qk,
                                         tp, inters.i_info(), inters.d_info());

                replace_operations_i(tp.operations[0].operation, tp.operations[1].operation);

                AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());
                *out++ = tp;
            }
            break;
            case 't' :
            {
                // Both touch (both arrive there)
                if ( get_turn_info_for_endpoint<false, true>(
                        pi, pj, pk, qi, qj, qk,
                        is_p_first, is_p_last, is_q_first, is_q_last,
                        tp_model, inters, method_touch, out) )
                {
                    // do nothing
                }
                else 
                {
                    touch<TurnInfo>::apply(pi, pj, pk, qi, qj, qk,
                            tp, inters.i_info(), inters.d_info(), inters.sides());

                    if ( tp.operations[1].operation == operation_blocked )
                    {
                        tp.operations[0].is_collinear = true;
                    }

                    // workarounds for touch<> not taking spikes into account starts here
                    // those was discovered empirically
                    // touch<> is not symmetrical!
                    // P spikes and Q spikes may produce various operations!
                    // Only P spikes are valid for L/A
                    // TODO: this is not optimal solution - think about rewriting touch<>

                    if ( tp.operations[0].operation == operation_blocked )
                    {
                        // a spike on P on the same line with Q1
                        if ( inters.is_spike_p() )
                        {
                            if ( inters.sides().qk_wrt_p1() == 0 )
                            {
                                tp.operations[0].is_collinear = true;
                            }
                            else
                            {
                                tp.operations[0].operation = operation_union;                                
                            }
                        }
                    }
                    else if ( tp.operations[0].operation == operation_continue
                           && tp.operations[1].operation == operation_continue )
                    {
                        // P spike on the same line with Q2 (opposite)
                        if ( inters.sides().pk_wrt_q1() == -inters.sides().qk_wrt_q1()
                          && inters.is_spike_p() )
                        {
                            tp.operations[0].operation = operation_union;
                            tp.operations[1].operation = operation_union;
                        }
                    }
                    else if ( tp.operations[0].operation == operation_none
                           && tp.operations[1].operation == operation_none )
                    {
                        // spike not handled by touch<>
                        if ( inters.is_spike_p() )
                        {
                            tp.operations[0].operation = operation_intersection;
                            tp.operations[1].operation = operation_union;

                            if ( inters.sides().pk_wrt_q2() == 0 )
                            {
                                tp.operations[0].operation = operation_continue; // will be converted to i
                                tp.operations[0].is_collinear = true;
                            }
                        }
                    }

                    // workarounds for touch<> not taking spikes into account ends here

                    replace_method_and_operations_tm(tp.method,
                                                     tp.operations[0].operation,
                                                     tp.operations[1].operation);

                    bool ignore_spike
                        = calculate_spike_operation(tp.operations[0].operation,
                                                    inters, is_p_last);

// TODO: move this into the append_xxx and call for each turn?
                    AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());

                    if ( ! handle_spikes
                      || ignore_spike
                      || ! append_opposite_spikes<append_touches>( // for 'i' or 'c' i???
                                tp, inters, is_p_last, is_q_last, out) )
                    {
                        *out++ = tp;
                    }
                }
            }
            break;
            case 'e':
            {
                if ( get_turn_info_for_endpoint<true, true>(
                        pi, pj, pk, qi, qj, qk,
                        is_p_first, is_p_last, is_q_first, is_q_last,
                        tp_model, inters, method_equal, out) )
                {
                    // do nothing
                }
                else
                {
                    tp.operations[0].is_collinear = true;

                    if ( ! inters.d_info().opposite )
                    {
                        // Both equal
                        // or collinear-and-ending at intersection point
                        equal<TurnInfo>::apply(pi, pj, pk, qi, qj, qk,
                            tp, inters.i_info(), inters.d_info(), inters.sides());

                        turn_transformer_ec<false> transformer(method_touch);
                        transformer(tp);
                    
// TODO: move this into the append_xxx and call for each turn?
                        AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());
                        
                        // conditionally handle spikes
                        if ( ! handle_spikes
                          || ! append_collinear_spikes(tp, inters, is_p_last, is_q_last,
                                                       method_touch, append_equal, out) )
                        {
                            *out++ = tp; // no spikes
                        }
                    }
                    else
                    {
                        equal_opposite
                            <
                                TurnInfo,
                                AssignPolicy
                            >::apply(pi, qi,
                                     tp, out, inters.i_info(), inters.d_info());
                    }
                }
            }
            break;
            case 'c' :
            {
                // Collinear
                if ( get_turn_info_for_endpoint<true, true>(
                        pi, pj, pk, qi, qj, qk,
                        is_p_first, is_p_last, is_q_first, is_q_last,
                        tp_model, inters, method_collinear, out) )
                {
                    // do nothing
                }
                else
                {
                    tp.operations[0].is_collinear = true;

                    if ( ! inters.d_info().opposite )
                    {
                        method_type method_replace = method_touch_interior;
                        append_version_c version = append_collinear;

                        if ( inters.d_info().arrival[0] == 0 )
                        {
                            // Collinear, but similar thus handled as equal
                            equal<TurnInfo>::apply(pi, pj, pk, qi, qj, qk,
                                    tp, inters.i_info(), inters.d_info(), inters.sides());

                            method_replace = method_touch;
                            version = append_equal;
                        }
                        else
                        {
                            collinear<TurnInfo>::apply(pi, pj, pk, qi, qj, qk,
                                    tp, inters.i_info(), inters.d_info(), inters.sides());

                            //method_replace = method_touch_interior;
                            //version = append_collinear;
                        }

                        turn_transformer_ec<false> transformer(method_replace);
                        transformer(tp);

// TODO: move this into the append_xxx and call for each turn?
                        AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());
                        
                        // conditionally handle spikes
                        if ( ! handle_spikes
                          || ! append_collinear_spikes(tp, inters, is_p_last, is_q_last,
                                                       method_replace, version, out) )
                        {
                            // no spikes
                            *out++ = tp;
                        }
                    }
                    else
                    {
                        // Is this always 'm' ?
                        turn_transformer_ec<false> transformer(method_touch_interior);

                        // conditionally handle spikes
                        if ( handle_spikes )
                        {
                            append_opposite_spikes<append_collinear_opposite>(
                                    tp, inters, is_p_last, is_q_last, out);
                        }

                        // TODO: ignore for spikes?
                        //       E.g. pass is_p_valid = !is_p_last && !is_pj_spike,
                        //       the same with is_q_valid

                        collinear_opposite
                            <
                                TurnInfo,
                                AssignPolicy
                            >::apply(pi, pj, pk, qi, qj, qk,
                                tp, out, inters.i_info(), inters.d_info(),
                                inters.sides(), transformer);
                    }
                }
            }
            break;
            case '0' :
            {
                // degenerate points
                if (AssignPolicy::include_degenerate)
                {
                    only_convert::apply(tp, inters.i_info());

                    if ( is_p_first
                      && equals::equals_point_point(pi, tp.point) )
                    {
                        tp.operations[0].position = position_front;
                    }
                    else if ( is_p_last
                           && equals::equals_point_point(pj, tp.point) )
                    {
                        tp.operations[0].position = position_back;
                    }
                    // tp.operations[1].position = position_middle;

                    AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());
                    *out++ = tp;
                }
            }
            break;
            default :
            {
#if defined(BOOST_GEOMETRY_DEBUG_ROBUSTNESS)
                std::cout << "TURN: Unknown method: " << method << std::endl;
#endif
#if ! defined(BOOST_GEOMETRY_OVERLAY_NO_THROW)
                throw turn_info_exception(method);
#endif
            }
            break;
        }

        return out;
    }

    template <typename Operation,
              typename IntersectionInfo>
    static inline bool calculate_spike_operation(Operation & op,
                                                 IntersectionInfo const& inters,
                                                 bool is_p_last)
    {
        bool is_p_spike = ( op == operation_union || op == operation_intersection )
                       && ! is_p_last
                       && inters.is_spike_p();

        if ( is_p_spike )
        {
            bool going_in = false, going_out = false;
                
            int const pk_q1 = inters.sides().pk_wrt_q1();
            int const pk_q2 = inters.sides().pk_wrt_q2();

            if ( inters.sides().qk_wrt_q1() <= 0 ) // Q turning R or C
            { 
                going_in = pk_q1 < 0 && pk_q2 < 0; // Pk on the right of both
                going_out = pk_q1 > 0 || pk_q2 > 0; // Pk on the left of one of them
            }
            else
            {
                going_in = pk_q1 < 0 || pk_q2 < 0; // Pk on the right of one of them
                going_out = pk_q1 > 0 && pk_q2 > 0; // Pk on the left of both
            }

            if ( going_in )
            {
                op = operation_intersection;
                return true;
            }
            else if ( going_out )
            {
                op = operation_union;
                return true;
            }
        }

        return false;
    }

    enum append_version_c { append_equal, append_collinear };

    template <typename TurnInfo,
              typename IntersectionInfo,
              typename OutIt>
    static inline bool append_collinear_spikes(TurnInfo & tp,
                                               IntersectionInfo const& inters,
                                               bool is_p_last, bool /*is_q_last*/,
                                               method_type method, append_version_c version,
                                               OutIt out)
    {
        // method == touch || touch_interior
        // both position == middle

        bool is_p_spike = ( version == append_equal ?
                            ( tp.operations[0].operation == operation_union
                           || tp.operations[0].operation == operation_intersection ) :
                            tp.operations[0].operation == operation_continue )
                       && ! is_p_last
                       && inters.is_spike_p();

        // TODO: throw an exception for spike in Areal?
        /*bool is_q_spike = tp.operations[1].operation == spike_op
                       && ! is_q_last
                       && inters.is_spike_q();*/

        if ( is_p_spike )
        {
            tp.method = method;
            tp.operations[0].operation = operation_blocked;
            tp.operations[1].operation = operation_union;
            *out++ = tp;
            tp.operations[0].operation = operation_continue; // boundary
            //tp.operations[1].operation = operation_union;
            *out++ = tp;

            return true;
        }
        
        return false;
    }

    enum append_version_o { append_touches, append_collinear_opposite };

    template <append_version_o Version,
              typename TurnInfo,
              typename IntersectionInfo,
              typename OutIt>
    static inline bool append_opposite_spikes(TurnInfo & tp,
                                              IntersectionInfo const& inters,
                                              bool is_p_last, bool /*is_q_last*/,
                                              OutIt out)
    {
        bool is_p_spike = ( Version == append_touches ?
                            ( tp.operations[0].operation == operation_continue
                           || tp.operations[0].operation == operation_intersection ) : // i ???
                            true )
                       && ! is_p_last
                       && inters.is_spike_p();
        // TODO: throw an exception for spike in Areal?
        /*bool is_q_spike = ( Version == append_touches ?
                            ( tp.operations[1].operation == operation_continue
                           || tp.operations[1].operation == operation_intersection ) :
                            true )
                       && ! is_q_last
                       && inters.is_spike_q();*/

        if ( is_p_spike && ( Version == append_touches || inters.d_info().arrival[0] == 1 ) )
        {
            if ( Version == append_touches )
            {
                tp.operations[0].is_collinear = true;
                //tp.operations[1].is_collinear = false;
                tp.method = method_touch;
            }
            else
            {
                tp.operations[0].is_collinear = true;
                //tp.operations[1].is_collinear = false;
                
                BOOST_ASSERT(inters.i_info().count > 1);
                base_turn_handler::assign_point(tp, method_touch_interior, inters.i_info(), 1);

                AssignPolicy::apply(tp, inters.pi(), inters.qi(), inters.i_info(), inters.d_info());
            }

            tp.operations[0].operation = operation_blocked;
            tp.operations[1].operation = operation_continue; // boundary
            *out++ = tp;
            tp.operations[0].operation = operation_continue; // boundary
            //tp.operations[1].operation = operation_continue; // boundary
            *out++ = tp;

            return true;
        }

        return false;
    }

    static inline void replace_method_and_operations_tm(method_type & method,
                                                        operation_type & op0,
                                                        operation_type & op1)
    {
        if ( op0 == operation_blocked && op1 == operation_blocked )
        {
            // NOTE: probably only if methods are WRT IPs, not segments!
            method = (method == method_touch ? method_equal : method_collinear);
        }

        // Assuming G1 is always Linear
        if ( op0 == operation_blocked )
        {
            op0 = operation_continue;
        }

        if ( op1 == operation_blocked )
        {
            op1 = operation_continue;
        }
        else if ( op1 == operation_intersection )
        {
            op1 = operation_union;
        }

        // spikes in 'm'
        if ( method == method_error )
        {
            method = method_touch_interior;
            op0 = operation_union;
            op1 = operation_union;
        }
    }

    template <bool IsFront>
    class turn_transformer_ec
    {
    public:
        explicit turn_transformer_ec(method_type method_t_or_m)
            : m_method(method_t_or_m)
        {}

        template <typename Turn>
        void operator()(Turn & turn) const
        {
            operation_type & op0 = turn.operations[0].operation;
            operation_type & op1 = turn.operations[1].operation;

            // NOTE: probably only if methods are WRT IPs, not segments!
            if ( IsFront
              || op0 == operation_intersection || op0 == operation_union
              || op1 == operation_intersection || op1 == operation_union )
            {
                turn.method = m_method;
            }

            turn.operations[0].is_collinear = op0 != operation_blocked;

            // Assuming G1 is always Linear
            if ( op0 == operation_blocked )
            {
                op0 = operation_continue;
            }

            if ( op1 == operation_blocked )
            {
                op1 = operation_continue;
            }
            else if ( op1 == operation_intersection )
            {
                op1 = operation_union;
            }
        }

    private:
        method_type m_method;
    };

    static inline void replace_operations_i(operation_type & /*op0*/, operation_type & op1)
    {
        // assuming Linear is always the first one
        op1 = operation_union;
    }

    // NOTE: Spikes may NOT be handled for Linear endpoints because it's not
    //       possible to define a spike on an endpoint. Areal geometries must
    //       NOT have spikes at all. One thing that could be done is to throw
    //       an exception when spike is detected in Areal geometry.
    
    template <bool EnableFirst,
              bool EnableLast,
              typename Point1,
              typename Point2,
              typename TurnInfo,
              typename IntersectionInfo,
              typename OutputIterator>
    static inline bool get_turn_info_for_endpoint(
                            Point1 const& pi, Point1 const& /*pj*/, Point1 const& /*pk*/,
                            Point2 const& qi, Point2 const& /*qj*/, Point2 const& /*qk*/,
                            bool is_p_first, bool is_p_last,
                            bool /*is_q_first*/, bool is_q_last,
                            TurnInfo const& tp_model,
                            IntersectionInfo const& inters,
                            method_type /*method*/,
                            OutputIterator out)
    {
        namespace ov = overlay;
        typedef ov::get_turn_info_for_endpoint<AssignPolicy, EnableFirst, EnableLast> get_info_e;

        const std::size_t ip_count = inters.i_info().count;
        // no intersection points
        if ( ip_count == 0 )
            return false;

        const int segment_index0 = tp_model.operations[0].seg_id.segment_index;
        const int segment_index1 = tp_model.operations[1].seg_id.segment_index;
        BOOST_ASSERT(segment_index0 >= 0 && segment_index1 >= 0);

        if ( !is_p_first && !is_p_last )
            return false;

// TODO: is_q_last could probably be replaced by false and removed from parameters

        linear_intersections intersections(pi, qi, inters.result(), is_p_last, is_q_last);
        linear_intersections::ip_info const& ip0 = intersections.template get<0>();
        linear_intersections::ip_info const& ip1 = intersections.template get<1>();

        const bool opposite = inters.d_info().opposite;

        // ANALYSE AND ASSIGN FIRST

        // IP on the first point of Linear Geometry
        if ( EnableFirst && is_p_first && ip0.is_pi && !ip0.is_qi ) // !q0i prevents duplication
        {
            TurnInfo tp = tp_model;
            tp.operations[0].position = position_front;
            tp.operations[1].position = position_middle;

            if ( opposite ) // opposite -> collinear
            {
                tp.operations[0].operation = operation_continue;
                tp.operations[1].operation = operation_union;
                tp.method = ip0.is_qj ? method_touch : method_touch_interior;
            }
            else
            {
                method_type replaced_method = method_touch_interior;

                if ( ip0.is_qj )
                {
                    side_calculator
                        <
                            typename IntersectionInfo::robust_point1_type,
                            typename IntersectionInfo::robust_point2_type,
                            typename IntersectionInfo::robust_point2_type
                        > side_calc(inters.rqi(), inters.rpi(), inters.rpj(),
                                    inters.rqi(), inters.rqj(), inters.rqk());

                    std::pair<operation_type, operation_type>
                        operations = get_info_e::operations_of_equal(side_calc);

                    tp.operations[0].operation = operations.first;
                    tp.operations[1].operation = operations.second;

                    replaced_method = method_touch;
                }
                else
                {
                    side_calculator
                        <
                            typename IntersectionInfo::robust_point1_type,
                            typename IntersectionInfo::robust_point2_type,
                            typename IntersectionInfo::robust_point2_type,
                            typename IntersectionInfo::robust_point1_type,
                            typename IntersectionInfo::robust_point1_type,
                            typename IntersectionInfo::robust_point2_type,
                            typename IntersectionInfo::robust_point1_type,
                            typename IntersectionInfo::robust_point2_type
                        > side_calc(inters.rqi(), inters.rpi(), inters.rpj(),
                                    inters.rqi(), inters.rpi(), inters.rqj());

                    std::pair<operation_type, operation_type>
                        operations = get_info_e::operations_of_equal(side_calc);

                    tp.operations[0].operation = operations.first;
                    tp.operations[1].operation = operations.second;
                }

                turn_transformer_ec<true> transformer(replaced_method);
                transformer(tp);
            }

            // equals<> or collinear<> will assign the second point,
            // we'd like to assign the first one
            base_turn_handler::assign_point(tp, tp.method, inters.i_info(), 0);

            // NOTE: is_collinear is not set for the first endpoint of L
            // for which there is no preceding segment
            // here is_p_first_ip == true
            tp.operations[0].is_collinear = false;

            AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());
            *out++ = tp;
        }

        // ANALYSE AND ASSIGN LAST

        // IP on the last point of Linear Geometry
        if ( EnableLast
          && is_p_last
          && ( ip_count > 1 ? (ip1.is_pj && !ip1.is_qi) : (ip0.is_pj && !ip0.is_qi) ) ) // prevents duplication
        {
            TurnInfo tp = tp_model;
            
            if ( inters.i_info().count > 1 )
            {
                //BOOST_ASSERT( result.template get<1>().dir_a == 0 && result.template get<1>().dir_b == 0 );
                tp.operations[0].is_collinear = true;
                tp.operations[1].operation = opposite ? operation_continue : operation_union;
            }
            else //if ( result.template get<0>().count == 1 )
            {
                side_calculator
                    <
                        typename IntersectionInfo::robust_point1_type,
                        typename IntersectionInfo::robust_point2_type,
                        typename IntersectionInfo::robust_point2_type
                    > side_calc(inters.rqi(), inters.rpj(), inters.rpi(),
                                inters.rqi(), inters.rqj(), inters.rqk());

                std::pair<operation_type, operation_type>
                    operations = get_info_e::operations_of_equal(side_calc);

                tp.operations[0].operation = operations.first;
                tp.operations[1].operation = operations.second;

                turn_transformer_ec<false> transformer(method_none);
                transformer(tp);

                tp.operations[0].is_collinear = tp.both(operation_continue);
            }

            tp.method = ( ip_count > 1 ? ip1.is_qj : ip0.is_qj ) ? method_touch : method_touch_interior;
            tp.operations[0].operation = operation_blocked;
            tp.operations[0].position = position_back;
            tp.operations[1].position = position_middle;
            
            // equals<> or collinear<> will assign the second point,
            // we'd like to assign the first one
            int ip_index = ip_count > 1 ? 1 : 0;
            base_turn_handler::assign_point(tp, tp.method, inters.i_info(), ip_index);

            AssignPolicy::apply(tp, pi, qi, inters.i_info(), inters.d_info());
            *out++ = tp;

            return true;
        }

        // don't ignore anything for now
        return false;
    }
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_LA_HPP
