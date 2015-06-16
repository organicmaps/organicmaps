// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2013-2015 Adam Wulkiewicz, Lodz, Poland

// This file was modified by Oracle on 2015.
// Modifications copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP


#include <deque>
#include <map>

#include <boost/range.hpp>
#include <boost/mpl/assert.hpp>


#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/overlay_type.hpp>
#include <boost/geometry/algorithms/detail/overlay/traverse.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>

#include <boost/geometry/algorithms/detail/recalculate.hpp>

#include <boost/geometry/algorithms/is_empty.hpp>
#include <boost/geometry/algorithms/reverse.hpp>

#include <boost/geometry/algorithms/detail/overlay/add_rings.hpp>
#include <boost/geometry/algorithms/detail/overlay/assign_parents.hpp>
#include <boost/geometry/algorithms/detail/overlay/ring_properties.hpp>
#include <boost/geometry/algorithms/detail/overlay/select_rings.hpp>
#include <boost/geometry/algorithms/detail/overlay/do_reverse.hpp>

#include <boost/geometry/policies/robustness/segment_ratio_type.hpp>


#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
#  include <boost/geometry/io/dsv/write.hpp>
#endif


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template <typename TurnPoints, typename TurnInfoMap>
inline void get_ring_turn_info(TurnInfoMap& turn_info_map,
        TurnPoints const& turn_points)
{
    typedef typename boost::range_value<TurnPoints>::type turn_point_type;
    typedef typename turn_point_type::container_type container_type;

    for (typename boost::range_iterator<TurnPoints const>::type
            it = boost::begin(turn_points);
         it != boost::end(turn_points);
         ++it)
    {
        typename boost::range_value<TurnPoints>::type const& turn_info = *it;
        bool both_uu = turn_info.both(operation_union);
        bool skip = (turn_info.discarded || both_uu)
            && ! turn_info.any_blocked()
            && ! turn_info.both(operation_intersection)
            ;

        for (typename boost::range_iterator<container_type const>::type
                op_it = boost::begin(turn_info.operations);
            op_it != boost::end(turn_info.operations);
            ++op_it)
        {
            ring_identifier ring_id
                (
                    op_it->seg_id.source_index,
                    op_it->seg_id.multi_index,
                    op_it->seg_id.ring_index
                );

            if (! skip)
            {
                turn_info_map[ring_id].has_normal_turn = true;
            }
            else if (both_uu)
            {
                turn_info_map[ring_id].has_uu_turn = true;
            }
        }
    }
}


template
<
    typename GeometryOut, overlay_type Direction, bool ReverseOut,
    typename Geometry1, typename Geometry2,
    typename OutputIterator
>
inline OutputIterator return_if_one_input_is_empty(Geometry1 const& geometry1,
            Geometry2 const& geometry2,
            OutputIterator out)
{
    typedef std::deque
        <
            typename geometry::ring_type<GeometryOut>::type
        > ring_container_type;

    typedef ring_properties<typename geometry::point_type<Geometry1>::type> properties;

// Silence warning C4127: conditional expression is constant
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127)
#endif

    // Union: return either of them
    // Intersection: return nothing
    // Difference: return first of them
    if (Direction == overlay_intersection
        || (Direction == overlay_difference && geometry::is_empty(geometry1)))
    {
        return out;
    }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif


    std::map<ring_identifier, ring_turn_info> empty;
    std::map<ring_identifier, properties> all_of_one_of_them;

    select_rings<Direction>(geometry1, geometry2, empty, all_of_one_of_them);
    ring_container_type rings;
    assign_parents(geometry1, geometry2, rings, all_of_one_of_them);
    return add_rings<GeometryOut>(all_of_one_of_them, geometry1, geometry2, rings, out);
}


template
<
    typename Geometry1, typename Geometry2,
    bool Reverse1, bool Reverse2, bool ReverseOut,
    typename GeometryOut,
    overlay_type Direction
>
struct overlay
{
    template <typename RobustPolicy, typename OutputIterator, typename Strategy>
    static inline OutputIterator apply(
                Geometry1 const& geometry1, Geometry2 const& geometry2,
                RobustPolicy const& robust_policy,
                OutputIterator out,
                Strategy const& )
    {
        bool const is_empty1 = geometry::is_empty(geometry1);
        bool const is_empty2 = geometry::is_empty(geometry2);

        if (is_empty1 && is_empty2)
        {
            return out;
        }

        if (is_empty1 || is_empty2)
        {
            return return_if_one_input_is_empty
                <
                    GeometryOut, Direction, ReverseOut
                >(geometry1, geometry2, out);
        }

        typedef typename geometry::point_type<GeometryOut>::type point_type;
        typedef detail::overlay::traversal_turn_info
        <
            point_type,
            typename geometry::segment_ratio_type<point_type, RobustPolicy>::type
        > turn_info;
        typedef std::deque<turn_info> container_type;

        typedef std::deque
            <
                typename geometry::ring_type<GeometryOut>::type
            > ring_container_type;

        container_type turn_points;

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "get turns" << std::endl;
#endif
        detail::get_turns::no_interrupt_policy policy;
        geometry::get_turns
            <
                Reverse1, Reverse2,
                detail::overlay::assign_null_policy
            >(geometry1, geometry2, robust_policy, turn_points, policy);

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "enrich" << std::endl;
#endif
        typename Strategy::side_strategy_type side_strategy;
        geometry::enrich_intersection_points<Reverse1, Reverse2>(turn_points,
                Direction == overlay_union
                    ? geometry::detail::overlay::operation_union
                    : geometry::detail::overlay::operation_intersection,
                    geometry1, geometry2,
                    robust_policy,
                    side_strategy);

#ifdef BOOST_GEOMETRY_DEBUG_ASSEMBLE
std::cout << "traverse" << std::endl;
#endif
        // Traverse through intersection/turn points and create rings of them.
        // Note that these rings are always in clockwise order, even in CCW polygons,
        // and are marked as "to be reversed" below
        ring_container_type rings;
        traverse<Reverse1, Reverse2, Geometry1, Geometry2>::apply
                (
                    geometry1, geometry2,
                    Direction == overlay_union
                        ? geometry::detail::overlay::operation_union
                        : geometry::detail::overlay::operation_intersection,
                    robust_policy,
                    turn_points, rings
                );

        std::map<ring_identifier, ring_turn_info> turn_info_per_ring;
        get_ring_turn_info(turn_info_per_ring, turn_points);

        typedef ring_properties
        <
            typename geometry::point_type<GeometryOut>::type
        > properties;

        // Select all rings which are NOT touched by any intersection point
        std::map<ring_identifier, properties> selected_ring_properties;
        select_rings<Direction>(geometry1, geometry2, turn_info_per_ring,
                selected_ring_properties);

        // Add rings created during traversal
        {
            ring_identifier id(2, 0, -1);
            for (typename boost::range_iterator<ring_container_type>::type
                    it = boost::begin(rings);
                 it != boost::end(rings);
                 ++it)
            {
                selected_ring_properties[id] = properties(*it);
                selected_ring_properties[id].reversed = ReverseOut;
                id.multi_index++;
            }
        }

        assign_parents(geometry1, geometry2, rings, selected_ring_properties);

        return add_rings<GeometryOut>(selected_ring_properties, geometry1, geometry2, rings, out);
    }
};


}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_OVERLAY_HPP
