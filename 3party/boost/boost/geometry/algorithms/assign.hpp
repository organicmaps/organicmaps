// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_ASSIGN_HPP
#define BOOST_GEOMETRY_ALGORITHMS_ASSIGN_HPP


#include <cstddef>

#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/if.hpp>
#include <boost/numeric/conversion/bounds.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits.hpp>

#include <boost/geometry/algorithms/detail/assign_values.hpp>
#include <boost/geometry/algorithms/convert.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>


#include <boost/geometry/util/for_each_coordinate.hpp>


namespace boost { namespace geometry
{

/*!
\brief Assign a range of points to a linestring, ring or polygon
\note The point-type of the range might be different from the point-type of the geometry
\ingroup assign
\tparam Geometry \tparam_geometry
\tparam Range \tparam_range_point
\param geometry \param_geometry
\param range \param_range_point

\qbk{
[heading Notes]
[note Assign automatically clears the geometry before assigning (use append if you don't want that)]
[heading Example]
[assign_points] [assign_points_output]

[heading See also]
\* [link geometry.reference.algorithms.append append]
}
 */
template <typename Geometry, typename Range>
inline void assign_points(Geometry& geometry, Range const& range)
{
    concept::check<Geometry>();

    clear(geometry);
    geometry::append(geometry, range, -1, 0);
}


/*!
\brief assign to a box inverse infinite
\details The assign_inverse function initialize a 2D or 3D box with large coordinates, the
min corner is very large, the max corner is very small. This is a convenient starting point to
collect the minimum bounding box of a geometry.
\ingroup assign
\tparam Geometry \tparam_geometry
\param geometry \param_geometry

\qbk{
[heading Example]
[assign_inverse] [assign_inverse_output]

[heading See also]
\* [link geometry.reference.algorithms.make.make_inverse make_inverse]
}
 */
template <typename Geometry>
inline void assign_inverse(Geometry& geometry)
{
    concept::check<Geometry>();

    dispatch::assign_inverse
        <
            typename tag<Geometry>::type,
            Geometry
        >::apply(geometry);
}

/*!
\brief assign zero values to a box, point
\ingroup assign
\details The assign_zero function initializes a 2D or 3D point or box with coordinates of zero
\tparam Geometry \tparam_geometry
\param geometry \param_geometry

 */
template <typename Geometry>
inline void assign_zero(Geometry& geometry)
{
    concept::check<Geometry>();

    dispatch::assign_zero
        <
            typename tag<Geometry>::type,
            Geometry
        >::apply(geometry);
}


#ifndef DOXYGEN_NO_DETAIL
namespace detail 
{
// Note: this is moved to namespace detail because the names and parameter orders
// are not yet 100% clear.

/*!
\brief Assign the four points of a 2D box
\ingroup assign
\note The order is crucial. Most logical is LOWER, UPPER and sub-order LEFT, RIGHT
    so this is how it is implemented.
\tparam Box \tparam_box
\tparam Point \tparam_point
\param box \param_box
\param lower_left point being assigned to lower left coordinates of the box
\param lower_right point being assigned to lower right coordinates of the box
\param upper_left point being assigned to upper left coordinates of the box
\param upper_right point being assigned to upper right coordinates of the box

\qbk{
[heading Example]
[assign_box_corners] [assign_box_corners_output]
}
*/
template <typename Box, typename Point>
inline void assign_box_corners(Box const& box,
        Point& lower_left, Point& lower_right,
        Point& upper_left, Point& upper_right)
{
    concept::check<Box const>();
    concept::check<Point>();

    detail::assign::assign_box_2d_corner
            <min_corner, min_corner>(box, lower_left);
    detail::assign::assign_box_2d_corner
            <max_corner, min_corner>(box, lower_right);
    detail::assign::assign_box_2d_corner
            <min_corner, max_corner>(box, upper_left);
    detail::assign::assign_box_2d_corner
            <max_corner, max_corner>(box, upper_right);
}

template <bool Reverse, typename Box, typename Range>
inline void assign_box_corners_oriented(Box const& box, Range& corners)
{
    if (Reverse)
    {
        // make counterclockwise ll,lr,ur,ul
        assign_box_corners(box, corners[0], corners[1], corners[3], corners[2]);
    }
    else
    {
        // make clockwise ll,ul,ur,lr
        assign_box_corners(box, corners[0], corners[3], corners[1], corners[2]);
    }
}


/*!
\brief Assign a box or segment with the value of a point
\ingroup assign
\tparam Index indicates which box-corner, min_corner (0) or max_corner (1)
    or which point of segment (0/1)
\tparam Point \tparam_point
\tparam Geometry \tparam_box_or_segment
\param point \param_point
\param geometry \param_box_or_segment

\qbk{
[heading Example]
[assign_point_to_index] [assign_point_to_index_output]
}
*/
template <std::size_t Index, typename Geometry, typename Point>
inline void assign_point_to_index(Point const& point, Geometry& geometry)
{
    concept::check<Point const>();
    concept::check<Geometry>();

    detail::assign::assign_point_to_index
        <
            Geometry, Point, Index, 0, dimension<Geometry>::type::value
        >::apply(point, geometry);
}


/*!
\brief Assign a point with a point of a box or segment
\ingroup assign
\tparam Index indicates which box-corner, min_corner (0) or max_corner (1)
    or which point of segment (0/1)
\tparam Geometry \tparam_box_or_segment
\tparam Point \tparam_point
\param geometry \param_box_or_segment
\param point \param_point

\qbk{
[heading Example]
[assign_point_from_index] [assign_point_from_index_output]
}
*/
template <std::size_t Index, typename Point, typename Geometry>
inline void assign_point_from_index(Geometry const& geometry, Point& point)
{
    concept::check<Geometry const>();
    concept::check<Point>();

    detail::assign::assign_point_from_index
        <
            Geometry, Point, Index, 0, dimension<Geometry>::type::value
        >::apply(geometry, point);
}

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Assigns one geometry to another geometry
\details The assign algorithm assigns one geometry, e.g. a BOX, to another geometry, e.g. a RING. This only
if it is possible and applicable.
\ingroup assign
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry (target)
\param geometry2 \param_geometry (source)

\qbk{
[heading Example]
[assign] [assign_output]

[heading See also]
\* [link geometry.reference.algorithms.convert convert]
}
 */
template <typename Geometry1, typename Geometry2>
inline void assign(Geometry1& geometry1, Geometry2 const& geometry2)
{
    concept::check_concepts_and_equal_dimensions<Geometry1, Geometry2 const>();

    dispatch::convert
        <
            typename tag<Geometry2>::type,
            typename tag<Geometry1>::type,
            dimension<Geometry1>::type::value,
            Geometry2,
            Geometry1
        >::apply(geometry2, geometry1);
}


}} // namespace boost::geometry



#endif // BOOST_GEOMETRY_ALGORITHMS_ASSIGN_HPP
