// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTIPOINT_TO_RANGE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISTANCE_MULTIPOINT_TO_RANGE_HPP

#include <boost/range.hpp>

#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/point_type.hpp>

#include <boost/geometry/algorithms/dispatch/distance.hpp>

#include <boost/geometry/algorithms/detail/distance/single_to_multi.hpp>
#include <boost/geometry/algorithms/detail/distance/multi_to_multi.hpp>
#include <boost/geometry/algorithms/detail/distance/geometry_to_geometry_rtree.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/iterators/point_iterator.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace distance
{

template <typename MultiPoint1, typename MultiPoint2, typename Strategy>
struct multipoint_to_multipoint
{
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            typename point_type<MultiPoint1>::type,
            typename point_type<MultiPoint2>::type
        >::type return_type;   

    static inline return_type apply(MultiPoint1 const& multipoint1,
                                    MultiPoint2 const& multipoint2,
                                    Strategy const& strategy)
    {
        if ( boost::size(multipoint1) > boost::size(multipoint2) )

        {
            return multipoint_to_multipoint
                <
                    MultiPoint2, MultiPoint1, Strategy
                >::apply(multipoint2, multipoint1, strategy);
        }

        return point_range_to_geometry_rtree
            <
                typename point_type<MultiPoint1>::type,
                MultiPoint2,
                Strategy
            >::apply(points_begin(multipoint1), points_end(multipoint1),
                     multipoint2, strategy);
    }
};


}} // namespace detail::distance
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


namespace splitted_dispatch
{

// specializations of distance_single_to_multi for various geometry combinations

template<typename Linestring, typename MultiPoint, typename Strategy>
struct distance_single_to_multi
    <
        Linestring, MultiPoint, Strategy,
        linestring_tag, multi_point_tag,
        strategy_tag_distance_point_segment
    >
{
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            typename point_type<Linestring>::type,
            typename point_type<MultiPoint>::type
        >::type return_type;   

    static inline return_type apply(Linestring const& linestring,
                                    MultiPoint const& multipoint,
                                    Strategy const& strategy)
    {
        return detail::distance::point_range_to_geometry_rtree
            <
                typename point_type<MultiPoint>::type,
                Linestring,
                Strategy
            >::apply(geometry::points_begin(multipoint),
                     geometry::points_end(multipoint),
                     linestring, strategy);
        
    }
};




// specializations of distance_multi_to_multi for various geometry combinations


template <typename MultiPoint1, typename MultiPoint2, typename Strategy>
struct distance_multi_to_multi
    <
        MultiPoint1, MultiPoint2, Strategy,
        multi_point_tag, multi_point_tag,
        strategy_tag_distance_point_point
    > : detail::distance::multipoint_to_multipoint
        <
            MultiPoint1, MultiPoint2, Strategy
        >
{};



template
<
    typename MultiPoint,
    typename MultiLinestring,
    typename Strategy
>
struct distance_multi_to_multi
    <
        MultiPoint, MultiLinestring, Strategy,
        multi_point_tag, multi_linestring_tag,
        strategy_tag_distance_point_segment
    >
{
    typedef typename strategy::distance::services::return_type
        <
            Strategy,
            typename point_type<MultiPoint>::type,
            typename point_type<MultiLinestring>::type
        >::type return_type;   

    static inline return_type apply(MultiPoint const& multipoint,
                                    MultiLinestring const& multilinestring,
                                    Strategy const& strategy)
    {
        return detail::distance::point_range_to_geometry_rtree
            <
                typename point_type<MultiPoint>::type,
                MultiLinestring,
                Strategy
            >::apply(geometry::points_begin(multipoint),
                     geometry::points_end(multipoint),
                     multilinestring, strategy);
        
    }
};

} // namespace splitted_dispatch


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_DISTANCE_ALTERNATE_HPP
