// Boost.Geometry (aka GGL, Generic Geometry Library)

// Boost.SpatialIndex - geometry helper functions
//
// Copyright 2008 Federico J. Fernandez.
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_GGL_INDEX_RTREE_HELPERS_HPP
#define BOOST_GEOMETRY_GGL_INDEX_RTREE_HELPERS_HPP

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/core/point_type.hpp>

namespace boost { namespace geometry { namespace index {

/**
 * \brief Given two boxes, returns the minimal box that contains them
 */
// TODO: use geometry::expand
template <typename Box>
inline Box enlarge_box(Box const& b1, Box const& b2)
{
    // TODO: mloskot - Refactor to readable form. Fix VC++8.0 min/max warnings:
    //  warning C4002: too many actual parameters for macro 'min

    typedef typename geometry::point_type<Box>::type point_type;

    point_type pmin(
        geometry::get<min_corner, 0>(b1) < geometry::get<min_corner, 0>(b2)
            ? geometry::get<min_corner, 0>(b1) : geometry::get<min_corner, 0>(b2),
        geometry::get<min_corner, 1>(b1) < geometry::get<min_corner, 1>(b2)
            ? geometry::get<min_corner, 1>(b1) : geometry::get<min_corner, 1>(b2));

    point_type pmax(
        geometry::get<max_corner, 0>(b1) > geometry::get<max_corner, 0>(b2)
            ? geometry::get<max_corner, 0>(b1) : geometry::get<max_corner, 0>(b2),
        geometry::get<max_corner, 1>(b1) > geometry::get<max_corner, 1>(b2)
            ? geometry::get<max_corner, 1>(b1) : geometry::get<max_corner, 1>(b2));

    return Box(pmin, pmax);
}

/**
 * \brief Compute the area of the union of b1 and b2
 */
template <typename Box>
inline typename default_area_result<Box>::type compute_union_area(Box const& b1, Box const& b2)
{
    Box enlarged_box = enlarge_box(b1, b2);
    return geometry::area(enlarged_box);
}

/**
 * \brief Checks if boxes intersects
 */
// TODO: move to geometry::intersects
template <typename Box>
inline bool is_overlapping(Box const& b1, Box const& b2)
{
    return ! geometry::disjoint(b1, b2);
}

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_GGL_INDEX_RTREE_HELPERS_HPP
