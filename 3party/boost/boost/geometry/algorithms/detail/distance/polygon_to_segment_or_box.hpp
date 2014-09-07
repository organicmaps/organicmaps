// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POLYGON_TO_SEGMENT_OR_BOX_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POLYGON_TO_SEGMENT_OR_BOX_HPP

#include <boost/range.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/distance_comparable_to_regular.hpp>
#include <boost/geometry/strategies/tags.hpp>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/intersects.hpp>

#include <boost/geometry/util/math.hpp>

#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/algorithms/detail/distance/point_to_geometry.hpp>
#include <boost/geometry/algorithms/detail/distance/range_to_segment_or_box.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{


template <typename Polygon, typename SegmentOrBox, typename Strategy>
class polygon_to_segment_or_box
{
private:
    typedef typename strategy::distance::services::comparable_type
       <
           Strategy
       >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy,
            typename point_type<Polygon>::type,
            typename point_type<SegmentOrBox>::type
        >::type comparable_return_type;

public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            typename point_type<Polygon>::type,
            typename point_type<SegmentOrBox>::type
        >::type return_type;

    static inline return_type apply(Polygon const& polygon,
                                    SegmentOrBox const& segment_or_box,
                                    Strategy const& strategy)
    {
        typedef typename geometry::ring_type<Polygon>::type e_ring;
        typedef typename geometry::interior_type<Polygon>::type i_rings;
        typedef typename range_value<i_rings>::type i_ring;

        if ( geometry::intersects(polygon, segment_or_box) )
        {
            return 0;
        }

        e_ring const& ext_ring = geometry::exterior_ring<Polygon>(polygon);
        i_rings const& int_rings = geometry::interior_rings<Polygon>(polygon);

        comparable_strategy cstrategy =
            strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(strategy);


        comparable_return_type cd_min = range_to_segment_or_box
            <
                e_ring, SegmentOrBox, comparable_strategy
            >::apply(ext_ring, segment_or_box, cstrategy, false);

        typedef typename boost::range_iterator<i_rings const>::type iterator_type;
        for (iterator_type it = boost::begin(int_rings);
             it != boost::end(int_rings); ++it)
        {
            comparable_return_type cd = range_to_segment_or_box
                <
                    i_ring, SegmentOrBox, comparable_strategy
                >::apply(*it, segment_or_box, cstrategy, false);

            if ( cd < cd_min )
            {
                cd_min = cd;
            }
        }

        return strategy::distance::services::comparable_to_regular
            <
                comparable_strategy,
                Strategy,
                Polygon,
                SegmentOrBox
            >::apply(cd_min);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DETAIL
namespace dispatch
{


template <typename Polygon, typename Segment, typename Strategy>
struct distance
    <
        Polygon, Segment, Strategy, polygon_tag, segment_tag,
        strategy_tag_distance_point_segment, false
    >    
    : detail::distance::polygon_to_segment_or_box<Polygon, Segment, Strategy>
{};



template <typename Polygon, typename Box, typename Strategy>
struct distance
    <
        Polygon, Box, Strategy, polygon_tag, box_tag,
        strategy_tag_distance_point_segment, false
    >    
    : detail::distance::polygon_to_segment_or_box<Polygon, Box, Strategy>
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_POLYGON_TO_SEGMENT_OR_BOX_HPP
