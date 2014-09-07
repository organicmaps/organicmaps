// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_RANGE_TO_SEGMENT_OR_BOX_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_RANGE_TO_SEGMENT_OR_BOX_HPP

#include <vector>

#include <boost/range.hpp>

#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>
#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/distance.hpp>
#include <boost/geometry/strategies/distance_comparable_to_regular.hpp>
#include <boost/geometry/strategies/tags.hpp>

#include <boost/geometry/algorithms/detail/assign_box_corners.hpp>
#include <boost/geometry/algorithms/detail/assign_indexed_point.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/algorithms/num_points.hpp>

#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/algorithms/detail/distance/point_to_geometry.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{



template
<
    typename Range,
    typename SegmentOrBox,
    typename Strategy
>
class range_to_segment_or_box
{
private:
    typedef typename point_type<SegmentOrBox>::type segment_or_box_point;
    typedef typename point_type<Range>::type range_point;

    typedef typename strategy::distance::services::comparable_type
        <
            Strategy
        >::type comparable_strategy;

    typedef typename strategy::distance::services::return_type
        <
            comparable_strategy, range_point, segment_or_box_point
        >::type comparable_return_type;

    typedef typename strategy::distance::services::tag
       <
           comparable_strategy
       >::type comparable_strategy_tag;

    typedef dispatch::distance
        <
            segment_or_box_point, Range, comparable_strategy,
            point_tag, typename tag<Range>::type,
            comparable_strategy_tag, false
        > comparable_point_to_range;

    // compute distance of a point to a segment or a box
    template
    <
        typename Point,
        typename SegOrBoxPoints,
        typename ComparableStrategy,
        typename Tag
    >
    struct comparable_distance_point_to_segment_or_box
    {};

    template
    <
        typename Point,
        typename SegmentPoints,
        typename ComparableStrategy
    >
    struct comparable_distance_point_to_segment_or_box
        <
            Point, SegmentPoints, ComparableStrategy, segment_tag
        >
    {
        static inline
        comparable_return_type apply(Point const& point,
                                     SegmentPoints const& segment_points,
                                     ComparableStrategy const& strategy)
        {
            boost::ignore_unused_variable_warning(strategy);
            return strategy.apply(point, segment_points[0], segment_points[1]);
        }
    };

    template
    <
        typename Point,
        typename BoxPoints,
        typename ComparableStrategy
    >
    struct comparable_distance_point_to_segment_or_box
        <
            Point, BoxPoints, ComparableStrategy, box_tag
        >
    {
        static inline
        comparable_return_type apply(Point const& point,
                                     BoxPoints const& box_points,
                                     ComparableStrategy const& strategy)
        {
            return point_to_range
                <
                    Point, BoxPoints, open, ComparableStrategy
                >::apply(point, box_points, strategy);
        }
    };


    // assign the points of a segment or a box to a range
    template
    <
        typename SegOrBox,
        typename PointRange,
        typename Tag = typename tag<SegOrBox>::type
    >
    struct assign_segment_or_box_points
    {};


    template <typename Segment, typename PointRange>
    struct assign_segment_or_box_points<Segment, PointRange, segment_tag>
    {
        static inline void apply(Segment const& segment, PointRange& range)
        {
            detail::assign_point_from_index<0>(segment, range[0]);
            detail::assign_point_from_index<1>(segment, range[1]);
        }
    };

    template <typename Box, typename PointRange>
    struct assign_segment_or_box_points<Box, PointRange, box_tag>
    {
        static inline void apply(Box const& box, PointRange& range)
        {
            detail::assign_box_corners_oriented<true>(box, range);
        }
    };


public:
    typedef typename strategy::distance::services::return_type
        <
            Strategy, range_point, segment_or_box_point
        >::type return_type;

    static inline return_type
    apply(Range const& range, SegmentOrBox const& segment_or_box,
          Strategy const& strategy, bool check_intersection = true)
    {
        if ( check_intersection && geometry::intersects(range, segment_or_box) )
        {
            return 0;
        }

        comparable_strategy cstrategy =
            strategy::distance::services::get_comparable
                <
                    Strategy
                >::apply(strategy);


        // get all points of the segment or the box
        std::vector<segment_or_box_point>
            segment_or_box_points(geometry::num_points(segment_or_box));

        assign_segment_or_box_points
            <
                
                SegmentOrBox, 
                std::vector<segment_or_box_point>
            >::apply(segment_or_box, segment_or_box_points);

        // consider all distances from each endpoint of the segment or box
        // to the range
        typename std::vector<segment_or_box_point>::const_iterator it
            = segment_or_box_points.begin();
        comparable_return_type cd_min =
            comparable_point_to_range::apply(*it, range, cstrategy);

        for (++it; it != segment_or_box_points.end(); ++it)
        {
            comparable_return_type cd =
                comparable_point_to_range::apply(*it, range, cstrategy);
            if ( cd < cd_min )
            {
                cd_min = cd;
            }
        }

        // consider all distances of the points in the range to the
        // segment or box
        typedef typename range_iterator<Range const>::type iterator_type;
        for (iterator_type it = boost::begin(range); it != boost::end(range); ++it)
        {
            comparable_return_type cd =
                comparable_distance_point_to_segment_or_box
                    <
                        typename point_type<Range>::type,
                        std::vector<segment_or_box_point>,
                        comparable_strategy,
                        typename tag<SegmentOrBox>::type
                    >::apply(*it, segment_or_box_points, cstrategy);

            if ( cd < cd_min )
            {
                cd_min = cd;
            }
        }

        return strategy::distance::services::comparable_to_regular
            <
                comparable_strategy, Strategy,
                range_point, segment_or_box_point
            >::apply(cd_min);
    }

    static inline return_type
    apply(SegmentOrBox const& segment_or_box, Range const& range, 
          Strategy const& strategy, bool check_intersection = true)
    {
        return apply(range, segment_or_box, strategy, check_intersection);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{



template <typename Linestring, typename Segment, typename Strategy>
struct distance
    <
        Linestring, Segment, Strategy, linestring_tag, segment_tag,
        strategy_tag_distance_point_segment, false
    >
        : detail::distance::range_to_segment_or_box
            <
                Linestring, Segment, Strategy
            >
{};




template <typename Segment, typename Ring, typename Strategy>
struct distance
    <
        Segment, Ring, Strategy, segment_tag, ring_tag,
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::range_to_segment_or_box
        <
            Ring, Segment, Strategy
        >
{};




template <typename Linestring, typename Box, typename Strategy>
struct distance
    <
        Linestring, Box, Strategy, linestring_tag, box_tag,
        strategy_tag_distance_point_segment, false
    >
        : detail::distance::range_to_segment_or_box
            <
                Linestring, Box, Strategy
            >
{};




template <typename Ring, typename Box, typename Strategy>
struct distance
    <
        Ring, Box, Strategy, ring_tag, box_tag,
        strategy_tag_distance_point_segment, false
    >
    : detail::distance::range_to_segment_or_box
        <
            Ring, Box, Strategy
        >
{};




} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_RANGE_TO_SEGMENT_OR_BOX_HPP
