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

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTI_TO_MULTI_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTI_TO_MULTI_HPP

#include <boost/range.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/distance_comparable_to_regular.hpp>
#include <boost/geometry/strategies/tags.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>

#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/algorithms/detail/distance/single_to_multi.hpp>
#include <boost/geometry/algorithms/detail/distance/geometry_to_geometry_rtree.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{



template <typename Multi1, typename Multi2, typename Strategy>
class distance_multi_to_multi_generic
{
private:
    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy,
            typename point_type<Multi1>::type,
            typename point_type<Multi2>::type
        >::type comparable_return_type;

public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            typename point_type<Multi1>::type,
            typename point_type<Multi2>::type
        >::type return_type;

    static inline return_type apply(Multi1 const& multi1,
                Multi2 const& multi2, Strategy const& strategy)
    {
        comparable_return_type min_cdist = comparable_return_type();
        bool first = true;

        comparable_strategy cstrategy =
            strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(strategy);

        for(typename range_iterator<Multi1 const>::type it = boost::begin(multi1);
                it != boost::end(multi1);
                ++it, first = false)
        {
            comparable_return_type cdist =
                dispatch::splitted_dispatch::distance_single_to_multi
                    <
                        typename range_value<Multi1>::type,
                        Multi2,
                        comparable_strategy,
                        typename tag<typename range_value<Multi1>::type>::type,
                        typename tag<Multi2>::type,
                        typename strategy::distance::services::tag
                            <
                                comparable_strategy
                            >::type
                    >::apply(*it, multi2, cstrategy);
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
                comparable_strategy,
                Strategy,
                Multi1,
                Multi2
            >::apply(min_cdist);
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
    typename MultiGeometry1,
    typename MultiGeometry2,
    typename Strategy,
    typename Tag1,
    typename Tag2,
    typename StrategyTag
>
struct distance_multi_to_multi
    : not_implemented<MultiGeometry1, MultiGeometry2>
{};



template
<
    typename MultiPoint,
    typename MultiPolygon,
    typename Strategy
>
struct distance_multi_to_multi
    <
        MultiPoint, MultiPolygon, Strategy,
        multi_point_tag, multi_polygon_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::distance_multi_to_multi_generic
        <
            MultiPoint, MultiPolygon, Strategy
        >
{};



template
<
    typename MultiLinestring1,
    typename MultiLinestring2,
    typename Strategy
>
struct distance_multi_to_multi
    <
        MultiLinestring1, MultiLinestring2, Strategy,
        multi_linestring_tag, multi_linestring_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            MultiLinestring1, MultiLinestring2, Strategy
        >
{};


template
<
    typename MultiLinestring,
    typename MultiPolygon,
    typename Strategy
>
struct distance_multi_to_multi
    <
        MultiLinestring, MultiPolygon, Strategy,
        multi_linestring_tag, multi_polygon_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            MultiLinestring, MultiPolygon, Strategy
        >
{};


template <typename MultiPolygon1, typename MultiPolygon2, typename Strategy>
struct distance_multi_to_multi
    <
        MultiPolygon1, MultiPolygon2, Strategy,
        multi_polygon_tag, multi_polygon_tag,
        strategy_tag_distance_point_segment
    > : detail::distance::geometry_to_geometry_rtree
        <
            MultiPolygon1, MultiPolygon2, Strategy
        >
{};


} // namespace splitted_dispatch




template
<
    typename MultiGeometry1,
    typename MultiGeometry2,
    typename Strategy,
    typename StrategyTag
>
struct distance
    <
        MultiGeometry1, MultiGeometry2, Strategy, multi_tag, multi_tag,
        StrategyTag, false
    > : splitted_dispatch::distance_multi_to_multi
        <
            MultiGeometry1, MultiGeometry2, Strategy,
            typename geometry::tag<MultiGeometry1>::type,
            typename geometry::tag<MultiGeometry2>::type,
            StrategyTag
        >
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTI_TO_MULTI_HPP
