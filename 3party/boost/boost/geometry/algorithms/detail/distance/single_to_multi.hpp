// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_SINGLE_TO_MULTI_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_SINGLE_TO_MULTI_HPP

#include <boost/numeric/conversion/bounds.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/geometry_id.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/tags.hpp>

#include <boost/geometry/util/select_coordinate_type.hpp>
#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/algorithms/detail/for_each_range.hpp>
#include <boost/geometry/algorithms/detail/sections/range_by_section.hpp>
#include <boost/geometry/algorithms/detail/sections/sectionalize.hpp>

#include <boost/geometry/views/detail/range_type.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>
#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/for_each.hpp>
#include <boost/geometry/algorithms/within.hpp>

#include <boost/geometry/algorithms/detail/distance/geometry_to_geometry_rtree.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template<typename Geometry, typename MultiGeometry, typename Strategy>
class distance_single_to_multi_generic
{
private:
    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy,
            typename point_type<Geometry>::type,
            typename point_type<MultiGeometry>::type
        >::type comparable_return_type;

public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            typename point_type<Geometry>::type,
            typename point_type<MultiGeometry>::type
        >::type return_type;

    static inline return_type apply(Geometry const& geometry,
                                    MultiGeometry const& multi,
                                    Strategy const& strategy)
    {
        comparable_return_type min_cdist = comparable_return_type();
        bool first = true;

        comparable_strategy cstrategy =
            strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(strategy);

        for (typename range_iterator<MultiGeometry const>::type it = boost::begin(multi);
                it != boost::end(multi);
                ++it, first = false)
        {
            comparable_return_type cdist = dispatch::distance
                <
                    Geometry,
                    typename range_value<MultiGeometry>::type,
                    comparable_strategy
                >::apply(geometry, *it, cstrategy);

            if (first || cdist < min_cdist)
            {
                min_cdist = cdist;
                if ( geometry::math::equals(min_cdist, 0) )
                {
                    break;
                }
            }
        }

        return strategy::distance::services::comparable_to_regular
            <
                comparable_strategy, Strategy, Geometry, MultiGeometry
            >::apply(min_cdist);
    }
};



template <typename MultiGeometry, typename Geometry, typename Strategy>
struct distance_multi_to_single_generic
{
    typedef typename strategy::distance::services::return_type
                     <
                         Strategy,
                         typename point_type<MultiGeometry>::type,
                         typename point_type<Geometry>::type
                     >::type return_type;

    static inline return_type apply(MultiGeometry const& multi,
                                    Geometry const& geometry,
                                    Strategy const& strategy)
    {
        return distance_single_to_multi_generic
            <
                Geometry, MultiGeometry, Strategy
            >::apply(geometry, multi, strategy);
    }
};




}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


namespace splitted_dispatch
{


template
<
    typename Geometry,
    typename MultiGeometry,
    typename Strategy,
    typename GeometryTag,
    typename MultiGeometryTag,
    typename StrategyTag
>
struct distance_single_to_multi
    : not_implemented<Geometry, MultiGeometry>
{};



template<typename Point, typename MultiPoint, typename Strategy>
struct distance_single_to_multi
    <
        Point, MultiPoint, Strategy,
        point_tag, multi_point_tag,
        strategy_tag_distance_point_point
    > : detail::distance::distance_single_to_multi_generic
        <
            Point, MultiPoint, Strategy
        >
{};



template<typename Point, typename MultiLinestring, typename Strategy>
struct distance_single_to_multi
    <
        Point, MultiLinestring, Strategy,
        point_tag, multi_linestring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_single_to_multi_generic
        <
            Point, MultiLinestring, Strategy
        >
{};



template<typename Point, typename MultiPolygon, typename Strategy>
struct distance_single_to_multi
    <
        Point, MultiPolygon, Strategy,
        point_tag, multi_polygon_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_single_to_multi_generic
        <
            Point, MultiPolygon, Strategy
        >
{};



template<typename Linestring, typename MultiLinestring, typename Strategy>
struct distance_single_to_multi
    <
        Linestring, MultiLinestring, Strategy,
        linestring_tag, multi_linestring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            Linestring, MultiLinestring, Strategy
        >
{};



template <typename Linestring, typename MultiPolygon, typename Strategy>
struct distance_single_to_multi
    <
        Linestring, MultiPolygon, Strategy,
        linestring_tag, multi_polygon_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            Linestring, MultiPolygon, Strategy
        >
{};



template <typename Polygon, typename MultiPoint, typename Strategy>
struct distance_single_to_multi
    <
        Polygon, MultiPoint, Strategy,
        polygon_tag, multi_point_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_single_to_multi_generic
        <
            Polygon, MultiPoint, Strategy
        >
{};



template <typename Polygon, typename MultiLinestring, typename Strategy>
struct distance_single_to_multi
    <
        Polygon, MultiLinestring, Strategy,
        polygon_tag, multi_linestring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            Polygon, MultiLinestring, Strategy
        >
{};



template <typename Polygon, typename MultiPolygon, typename Strategy>
struct distance_single_to_multi
    <
        Polygon, MultiPolygon, Strategy,
        polygon_tag, multi_polygon_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            Polygon, MultiPolygon, Strategy
        >
{};



template <typename MultiLinestring, typename Ring, typename Strategy>
struct distance_single_to_multi
    <
        MultiLinestring, Ring, Strategy,
        multi_linestring_tag, ring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            MultiLinestring, Ring, Strategy
        >
{};


template <typename MultiPolygon, typename Ring, typename Strategy>
struct distance_single_to_multi
    <
        MultiPolygon, Ring, Strategy,
        multi_polygon_tag, ring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            MultiPolygon, Ring, Strategy
        >
{};



template
<
    typename MultiGeometry,
    typename Geometry,
    typename Strategy,
    typename MultiGeometryTag,
    typename GeometryTag,
    typename StrategyTag
>
struct distance_multi_to_single
    : not_implemented<MultiGeometry, Geometry>
{};



template
<
    typename MultiPoint,
    typename Segment,
    typename Strategy
>
struct distance_multi_to_single
    <
        MultiPoint, Segment, Strategy,
        multi_point_tag, segment_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_multi_to_single_generic
        <
            MultiPoint, Segment, Strategy
        >
{};



template
<
    typename MultiPoint,
    typename Ring,
    typename Strategy
>
struct distance_multi_to_single
    <
        MultiPoint, Ring, Strategy,
        multi_point_tag, ring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_multi_to_single_generic
        <
            MultiPoint, Ring, Strategy
        >
{};



template
<
    typename MultiPoint,
    typename Box,
    typename Strategy
>
struct distance_multi_to_single
    <
        MultiPoint, Box, Strategy,
        multi_point_tag, box_tag,
        strategy_tag_distance_point_box
    > : detail::distance::distance_multi_to_single_generic
        <
            MultiPoint, Box, Strategy
        >
{};


template <typename MultiLinestring, typename Segment, typename Strategy>
struct distance_multi_to_single
    <
        MultiLinestring, Segment, Strategy,
        multi_linestring_tag, segment_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_multi_to_single_generic
        <
            MultiLinestring, Segment, Strategy
        >
{};


template <typename MultiLinestring, typename Ring, typename Strategy>
struct distance_multi_to_single
    <
        MultiLinestring, Ring, Strategy,
        multi_linestring_tag, ring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            MultiLinestring, Ring, Strategy
        >
{};


template <typename MultiLinestring, typename Box, typename Strategy>
struct distance_multi_to_single
    <
        MultiLinestring, Box, Strategy,
        multi_linestring_tag, box_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_multi_to_single_generic
        <
            MultiLinestring, Box, Strategy
        >
{};



template <typename MultiPolygon, typename Segment, typename Strategy>
struct distance_multi_to_single
    <
        MultiPolygon, Segment, Strategy,
        multi_polygon_tag, segment_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_multi_to_single_generic
        <
            MultiPolygon, Segment, Strategy
        >
{};


template <typename MultiPolygon, typename Ring, typename Strategy>
struct distance_multi_to_single
    <
        MultiPolygon, Ring, Strategy,
        multi_polygon_tag, ring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            MultiPolygon, Ring, Strategy
        >
{};


template <typename MultiPolygon, typename Box, typename Strategy>
struct distance_multi_to_single
    <
        MultiPolygon, Box, Strategy,
        multi_polygon_tag, box_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_multi_to_single_generic
        <
            MultiPolygon, Box, Strategy
        >
{};


} // namespace splitted_dispatch


template
<
    typename Geometry,
    typename MultiGeometry,
    typename Strategy,
    typename GeometryTag,
    typename StrategyTag
>
struct distance
    <
        Geometry, MultiGeometry, Strategy, GeometryTag, multi_tag,
        StrategyTag, false
    > : splitted_dispatch::distance_single_to_multi
        <
            Geometry, MultiGeometry, Strategy,
            GeometryTag, typename tag<MultiGeometry>::type,
            StrategyTag
        >
{};



template
<
    typename MultiGeometry,
    typename Geometry,
    typename Strategy,
    typename GeometryTag,
    typename StrategyTag
>
struct distance
    <
        MultiGeometry, Geometry, Strategy, multi_tag, GeometryTag,
        StrategyTag, false
    > : splitted_dispatch::distance_multi_to_single
        <
            MultiGeometry, Geometry, Strategy,
            typename tag<MultiGeometry>::type, GeometryTag,
            StrategyTag
        >
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_SINGLE_TO_MULTI_HPP
