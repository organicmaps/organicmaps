// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_OFFSET_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_OFFSET_HPP

#include <boost/config.hpp>

#include <boost/range/functions.hpp>

#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/algorithms/detail/buffer/buffer_inserter.hpp>
#include <boost/geometry/strategies/buffer.hpp>
#include <boost/geometry/strategies/agnostic/buffer_distance_asymmetric.hpp>
#include <boost/geometry/strategies/agnostic/buffer_end_skip.hpp>
#include <boost/geometry/strategies/cartesian/buffer_side.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/geometries/segment.hpp>

#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>

namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace offset
{


template
<
    typename Range,
    typename RangeOut
>
struct offset_range
{
    typedef geometry::detail::buffer::buffer_range
        <
            Range
        > per_range;

    template
    <
        typename Collection,
        typename DistanceStrategy,
        typename SideStrategy,
        typename JoinStrategy,
        typename EndStrategy,
        typename RobustPolicy
    >
    static inline void apply(Collection& collection, Range const& range,
                DistanceStrategy const& distance_strategy,
                SideStrategy const& side_strategy,
                JoinStrategy const& join_strategy,
                EndStrategy const& end_strategy,
                RobustPolicy const& robust_policy,
                bool reverse)
    {
        collection.start_new_ring();
        typedef typename point_type<RangeOut>::type output_point_type;
        output_point_type first_p1, first_p2, last_p1, last_p2;

        if (reverse)
        {
            per_range::iterate(collection, 0, boost::rbegin(range), boost::rend(range),
                strategy::buffer::buffer_side_left,
                distance_strategy, side_strategy, join_strategy, end_strategy, robust_policy,
                first_p1, first_p2, last_p1, last_p2);
        }
        else
        {
            per_range::iterate(collection, 0, boost::begin(range), boost::end(range),
                strategy::buffer::buffer_side_left,
                distance_strategy, side_strategy, join_strategy, end_strategy, robust_policy,
                first_p1, first_p2, last_p1, last_p2);
        }
        collection.finish_ring();
    }
};

}} // namespace detail::offset
#endif



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
struct offset
{};


template
<
    typename Geometry,
    typename GeometryOut
>
struct offset
    <
        linestring_tag,
        linestring_tag,
        Geometry,
        GeometryOut
    >
    : detail::offset::offset_range
        <
            Geometry,
            GeometryOut
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template
<
    typename Geometry,
    typename GeometryOut,
    typename JoinStrategy,
    typename Distance
>
inline void offset(Geometry const& geometry, GeometryOut& out,
            JoinStrategy const& join_strategy,
            Distance const& distance)
{
    concept::check<Geometry const>();
    concept::check<GeometryOut>();

    typedef typename geometry::point_type<Geometry>::type point_type;

    detail::no_rescale_policy robust_policy;

    detail::buffer::buffered_piece_collection
        <
            model::ring<point_type>,
            detail::no_rescale_policy
        > collection(robust_policy);

    bool reverse = distance < 0;
    strategy::buffer::distance_asymmetric
        <
            typename geometry::coordinate_type<Geometry>::type
        > distance_strategy(geometry::math::abs(distance),
                            geometry::math::abs(distance));

    strategy::buffer::end_skip
        <
            point_type,
            point_type
        > end_strategy;

    strategy::buffer::buffer_side side_strategy;

    dispatch::offset
        <
            typename tag<Geometry>::type,
            typename tag<GeometryOut>::type,
            Geometry,
            GeometryOut
        >::apply(collection,
                 geometry,
                 distance_strategy,
                 side_strategy,
                 join_strategy,
                 end_strategy,
                robust_policy,
                 reverse);

    // TODO collection.template assign<GeometryOut>(out);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_OFFSET_HPP
