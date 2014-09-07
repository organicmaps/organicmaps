// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.
// Copyright (c) 2013-2014 Adam Wulkiewicz, Lodz, Poland.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POINT_TO_GEOMETRY_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POINT_TO_GEOMETRY_HPP

#include <utility>

#include <boost/range.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/interior_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/tags.hpp>
#include <boost/geometry/strategies/distance_comparable_to_regular.hpp>

#include <boost/geometry/views/closeable_view.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/intersects.hpp>

#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/algorithms/detail/distance/default_strategies.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template <typename P1, typename P2, typename Strategy>
struct point_to_point
{
    static inline
    typename strategy::distance::services::return_type<Strategy, P1, P2>::type
    apply(P1 const& p1, P2 const& p2, Strategy const& strategy)
    {
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(p1, p2);
    }
};


template
<
    typename Point,
    typename Range,
    closure_selector Closure,
    typename Strategy
>
class point_to_range
{
private:
    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy,
            Point,
            typename boost::range_value<Range>::type
        >::type comparable_return_type;

public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            Point,
            typename boost::range_value<Range>::type
        >::type return_type;

    static inline return_type apply(Point const& point, Range const& range,
                                    Strategy const& strategy)
    {
        comparable_strategy c_strategy =
            strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(strategy);

        comparable_return_type const zero = comparable_return_type(0);

        if (boost::size(range) == 0)
        {
            return zero;
        }

        typedef typename closeable_view<Range const, Closure>::type view_type;

        view_type view(range);

        // line of one point: return point distance
        typedef typename boost::range_iterator<view_type const>::type iterator_type;
        iterator_type it = boost::begin(view);
        iterator_type prev = it++;
        if (it == boost::end(view))
        {
            return strategy::distance::services::comparable_to_regular
            <
                comparable_strategy, Strategy, Point,
                typename boost::range_value<Range>::type
            >::apply( c_strategy.apply(point,
                                       *boost::begin(view),
                                       *boost::begin(view)) );
        }

        // start with first segment distance
        comparable_return_type cd = c_strategy.apply(point, *prev, *it);

        // check if other segments are closer
        for (++prev, ++it; it != boost::end(view); ++prev, ++it)
        {
            comparable_return_type cds = c_strategy.apply(point, *prev, *it);
            if (geometry::math::equals(cds, zero))
            {
                return strategy::distance::services::comparable_to_regular
                    <
                        comparable_strategy,
                        Strategy,
                        Point,
                        typename boost::range_value<Range>::type
                    >::apply(zero);
            }
            else if (cds < cd)
            {
                cd = cds;
            }
        }

        return strategy::distance::services::comparable_to_regular
            <
                comparable_strategy,
                Strategy,
                Point,
                typename boost::range_value<Range>::type
            >::apply(cd);
    }
};


template
<
    typename Point,
    typename Ring,
    closure_selector Closure,
    typename Strategy
>
struct point_to_ring
{
    typedef std::pair
        <
            typename strategy::distance::services::return_type
                <
                    Strategy, Point, typename point_type<Ring>::type
                >::type,
            bool
        > distance_containment;

    static inline distance_containment apply(Point const& point,
                                             Ring const& ring,
                                             Strategy const& strategy)
    {
        return distance_containment
            (
                point_to_range
                    <
                        Point,
                        Ring,
                        Closure,
                        Strategy
                    >::apply(point, ring, strategy),
                geometry::within(point, ring)
            );
    }
};



template
<
    typename Point,
    typename Polygon,
    closure_selector Closure,
    typename Strategy
>
class point_to_polygon
{
private:
    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy, Point, typename point_type<Polygon>::type
        >::type comparable_return_type;

    typedef std::pair
        <
            comparable_return_type, bool
        > comparable_distance_containment;

public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy, Point, typename point_type<Polygon>::type
        >::type return_type;
    typedef std::pair<return_type, bool> distance_containment;

    static inline distance_containment apply(Point const& point,
                                             Polygon const& polygon,
                                             Strategy const& strategy)
    {
        comparable_strategy c_strategy =
            strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(strategy);

        // Check distance to all rings
        typedef point_to_ring
            <
                Point,
                typename ring_type<Polygon>::type,
                Closure,
                comparable_strategy
            > per_ring;

        comparable_distance_containment dc =
            per_ring::apply(point, exterior_ring(polygon), c_strategy);

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(polygon);
        for (typename boost::range_iterator
                 <
                     typename interior_type<Polygon const>::type const
                 >::type it = boost::begin(rings);
             it != boost::end(rings); ++it)
        {
            comparable_distance_containment dcr =
                per_ring::apply(point, *it, c_strategy);
            if (dcr.first < dc.first)
            {
                dc.first = dcr.first;
            }
            // If it was inside, and also inside inner ring,
            // turn off the inside-flag, it is outside the polygon
            if (dc.second && dcr.second)
            {
                dc.second = false;
            }
        }

        return_type rd = strategy::distance::services::comparable_to_regular
            <
                comparable_strategy, Strategy, Point, Polygon
            >::apply(dc.first);

        return std::make_pair(rd, dc.second);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL




#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// Point-point
template <typename P1, typename P2, typename Strategy>
struct distance
    <
        P1, P2, Strategy,
        point_tag, point_tag, strategy_tag_distance_point_point,
        false
    >
    : detail::distance::point_to_point<P1, P2, Strategy>
{};


// Point-line version 2, where point-segment strategy is specified
template <typename Point, typename Linestring, typename Strategy>
struct distance
<
    Point, Linestring, Strategy,
    point_tag, linestring_tag, strategy_tag_distance_point_segment,
    false
> : detail::distance::point_to_range<Point, Linestring, closed, Strategy>
{};


// Point-ring , where point-segment strategy is specified
template <typename Point, typename Ring, typename Strategy>
struct distance
<
    Point, Ring, Strategy,
    point_tag, ring_tag, strategy_tag_distance_point_segment,
    false
>
{
    typedef typename strategy::distance::services::return_type
        <
            Strategy, Point, typename point_type<Ring>::type
        >::type return_type;

    static inline return_type apply(Point const& point,
                                    Ring const& ring,
                                    Strategy const& strategy)
    {
        std::pair<return_type, bool>
            dc = detail::distance::point_to_ring
            <
                Point, Ring,
                geometry::closure<Ring>::value,
                Strategy
            >::apply(point, ring, strategy);

        return dc.second ? return_type(0) : dc.first;
    }
};


// Point-polygon , where point-segment strategy is specified
template <typename Point, typename Polygon, typename Strategy>
struct distance
<
    Point, Polygon, Strategy, point_tag, polygon_tag, 
    strategy_tag_distance_point_segment, false
>
{
    typedef typename strategy::distance::services::return_type
        <
            Strategy, Point, typename point_type<Polygon>::type
        >::type return_type;

    static inline return_type apply(Point const& point,
                                    Polygon const& polygon,
                                    Strategy const& strategy)
    {
        std::pair<return_type, bool>
            dc = detail::distance::point_to_polygon
            <
                Point, Polygon,
                geometry::closure<Polygon>::value,
                Strategy
            >::apply(point, polygon, strategy);

        return dc.second ? return_type(0) : dc.first;
    }
};


// Point-segment version 2, with point-segment strategy
template <typename Point, typename Segment, typename Strategy>
struct distance
<
    Point, Segment, Strategy,
    point_tag, segment_tag, strategy_tag_distance_point_segment,
    false
>
{
    static inline typename return_type<Strategy, Point, typename point_type<Segment>::type>::type
    apply(Point const& point,
          Segment const& segment,
          Strategy const& strategy)
    {
        typename point_type<Segment>::type p[2];
        geometry::detail::assign_point_from_index<0>(segment, p[0]);
        geometry::detail::assign_point_from_index<1>(segment, p[1]);

        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(point, p[0], p[1]);
    }
};



template <typename Point, typename Box, typename Strategy>
struct distance
    <
         Point, Box, Strategy, point_tag, box_tag,
         strategy_tag_distance_point_box, false
    >
{
    static inline typename strategy::distance::services::return_type
        <
            Strategy, Point, typename point_type<Box>::type
        >::type
    apply(Point const& point, Box const& box, Strategy const& strategy)
    {
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(point, box);
    }
};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POINT_TO_GEOMETRY_HPP
