// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DISSOLVE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DISSOLVE_HPP


#include <map>
#include <vector>

#include <boost/range.hpp>

#include <boost/geometry/algorithms/detail/overlay/get_turns.hpp>
#include <boost/geometry/algorithms/detail/overlay/self_turn_points.hpp>

#include <boost/geometry/algorithms/detail/overlay/turn_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/enrichment_info.hpp>
#include <boost/geometry/algorithms/detail/overlay/traversal_info.hpp>

#include <boost/geometry/algorithms/detail/point_on_border.hpp>

#include <boost/geometry/algorithms/detail/overlay/enrich_intersection_points.hpp>
#include <boost/geometry/algorithms/detail/overlay/traverse.hpp>

#include <boost/geometry/algorithms/detail/overlay/add_rings.hpp>
#include <boost/geometry/algorithms/detail/overlay/assign_parents.hpp>
#include <boost/geometry/algorithms/detail/overlay/ring_properties.hpp>
#include <boost/geometry/algorithms/detail/overlay/select_rings.hpp>

#include <boost/geometry/algorithms/convert.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace dissolve
{

struct no_interrupt_policy
{
    static bool const enabled = false;
    static bool const has_intersections = false;


    template <typename Range>
    static inline bool apply(Range const&)
    {
        return false;
    }
};


template<typename Geometry>
class backtrack_for_dissolve
{
public :
    typedef detail::overlay::backtrack_state state_type;

    template <typename Operation, typename Rings, typename Turns, typename RescalePolicy>
    static inline void apply(std::size_t size_at_start,
                Rings& rings, typename boost::range_value<Rings>::type& ring,
                Turns& turns, Operation& operation,
                std::string const& ,
                Geometry const& ,
                Geometry const& ,
                RescalePolicy const& ,
                state_type& state
                )
    {
        state.m_good = false;

        // Make bad output clean
        rings.resize(size_at_start);
        ring.clear();

        // Reject this as a starting point
        operation.visited.set_rejected();

        // And clear all visit info
        clear_visit_info(turns);
    }
};


template <typename Geometry, typename GeometryOut>
struct dissolve_ring_or_polygon
{
    template <typename RescalePolicy, typename OutputIterator>
    static inline OutputIterator apply(Geometry const& geometry,
                RescalePolicy const& rescale_policy,
                OutputIterator out)
    {
        typedef typename point_type<Geometry>::type point_type;

        // Get the self-intersection points, including turns
        typedef detail::overlay::traversal_turn_info
            <
                point_type,
                typename segment_ratio_type<point_type, RescalePolicy>::type
            > turn_info;

        std::vector<turn_info> turns;
        detail::dissolve::no_interrupt_policy policy;
        geometry::self_turns
            <
                detail::overlay::assign_null_policy
            >(geometry, rescale_policy, turns, policy);

        // The dissolve process is not necessary if there are no turns at all

        if (boost::size(turns) > 0)
        {
            typedef typename ring_type<Geometry>::type ring_type;
            typedef std::vector<ring_type> out_vector;
            out_vector rings;

            // Enrich the turns
            typedef typename strategy::side::services::default_strategy
            <
                typename cs_tag<Geometry>::type
            >::type side_strategy_type;

            enrich_intersection_points<false, false>(turns,
                        detail::overlay::operation_union,
                        geometry, geometry, rescale_policy,
                        side_strategy_type());

            typedef detail::overlay::traverse
                <
                    false, false,
                    Geometry, Geometry,
                    backtrack_for_dissolve<Geometry>
                > traverser;


            // Traverse the polygons twice for union...
            traverser::apply(geometry, geometry,
                            detail::overlay::operation_union,
                            rescale_policy,
                            turns, rings);

            clear_visit_info(turns);

            enrich_intersection_points<false, false>(turns,
                        detail::overlay::operation_intersection,
                        geometry, geometry, rescale_policy,
                        side_strategy_type());

            // ... and for intersection
            traverser::apply(geometry, geometry,
                            detail::overlay::operation_intersection,
                            rescale_policy,
                            turns, rings);

            std::map<ring_identifier, detail::overlay::ring_turn_info> map;
            get_ring_turn_info(map, turns);

            typedef detail::overlay::ring_properties<typename geometry::point_type<Geometry>::type> properties;

            std::map<ring_identifier, properties> selected;

            detail::overlay::select_rings<overlay_union>(geometry, map, selected);

            // Add intersected rings
            {
                ring_identifier id(2, 0, -1);
                for (typename boost::range_iterator<std::vector<ring_type> const>::type
                        it = boost::begin(rings);
                        it != boost::end(rings);
                        ++it)
                {
                    selected[id] = properties(*it);
                    id.multi_index++;
                }
            }

            detail::overlay::assign_parents(geometry, rings, selected, true);
            return detail::overlay::add_rings<GeometryOut>(selected, geometry, rings, out);

        }
        else
        {
            GeometryOut g;
            geometry::convert(geometry, g);
            *out++ = g;
            return out;
        }
    }
};



}} // namespace detail::dissolve
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename GeometryTag,
    typename GeometryOutTag,
    typename Geometry,
    typename GeometryOut
>
struct dissolve
    : not_implemented<GeometryTag, GeometryOutTag>
{};


template<typename Polygon, typename PolygonOut>
struct dissolve<polygon_tag, polygon_tag, Polygon, PolygonOut>
    : detail::dissolve::dissolve_ring_or_polygon<Polygon, PolygonOut>
{};


template<typename Ring, typename RingOut>
struct dissolve<ring_tag, ring_tag, Ring, RingOut>
    : detail::dissolve::dissolve_ring_or_polygon<Ring, RingOut>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



/*!
    \brief Removes self intersections from a geometry
    \ingroup overlay
    \tparam Geometry geometry type
    \tparam OutputIterator type of intersection container
        (e.g. vector of "intersection/turn point"'s)
    \param geometry first geometry
    \param out output iterator getting dissolved geometry
    \note Currently dissolve with a (multi)linestring does NOT remove internal
        overlap, it only tries to connect multiple line end-points.
        TODO: we should change this behaviour and add a separate "connect"
        algorithm, and let dissolve work like polygon.
 */
template
<
    typename GeometryOut,
    typename Geometry,
    typename OutputIterator
>
inline OutputIterator dissolve_inserter(Geometry const& geometry, OutputIterator out)
{
    concept::check<Geometry const>();
    concept::check<GeometryOut>();

    typedef typename geometry::rescale_policy_type
    <
        typename geometry::point_type<Geometry>::type
    >::type rescale_policy_type;

    rescale_policy_type robust_policy
        = geometry::get_rescale_policy<rescale_policy_type>(geometry);

    return dispatch::dissolve
    <
        typename tag<Geometry>::type,
        typename tag<GeometryOut>::type,
        Geometry,
        GeometryOut
    >::apply(geometry, robust_policy, out);
}


template
<
    typename Geometry,
    typename Collection
>
inline void dissolve(Geometry const& geometry, Collection& output_collection)
{
    concept::check<Geometry const>();

    typedef typename boost::range_value<Collection>::type geometry_out;

    concept::check<geometry_out>();

    dispatch::dissolve
    <
        typename tag<Geometry>::type,
        typename tag<geometry_out>::type,
        Geometry,
        geometry_out
    >::apply(geometry, detail::no_rescale_policy(), std::back_inserter(output_collection));
}



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_DISSOLVE_HPP
