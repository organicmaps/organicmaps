// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_HPP

// Note: contrary to most files, the geometry::detail::disjoint namespace
// is partly implemented in a separate file, to avoid circular references
// disjoint -> get_turns -> disjoint

#include <cstddef>

#include <boost/range.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>

#include <boost/geometry/algorithms/covered_by.hpp>

#include <boost/geometry/util/math.hpp>



namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{


struct disjoint_interrupt_policy
{
    static bool const enabled = true;
    bool has_intersections;

    inline disjoint_interrupt_policy()
        : has_intersections(false)
    {}

    template <typename Range>
    inline bool apply(Range const& range)
    {
        // If there is any IP in the range, it is NOT disjoint
        if (boost::size(range) > 0)
        {
            has_intersections = true;
            return true;
        }
        return false;
    }
};



template
<
    typename Point1, typename Point2,
    std::size_t Dimension, std::size_t DimensionCount
>
struct point_point
{
    static inline bool apply(Point1 const& p1, Point2 const& p2)
    {
        if (! geometry::math::equals(get<Dimension>(p1), get<Dimension>(p2)))
        {
            return true;
        }
        return point_point
            <
                Point1, Point2,
                Dimension + 1, DimensionCount
            >::apply(p1, p2);
    }
};


template <typename Point1, typename Point2, std::size_t DimensionCount>
struct point_point<Point1, Point2, DimensionCount, DimensionCount>
{
    static inline bool apply(Point1 const& , Point2 const& )
    {
        return false;
    }
};


template
<
    typename Point, typename Box,
    std::size_t Dimension, std::size_t DimensionCount
>
struct point_box
{
    static inline bool apply(Point const& point, Box const& box)
    {
        if (get<Dimension>(point) < get<min_corner, Dimension>(box)
            || get<Dimension>(point) > get<max_corner, Dimension>(box))
        {
            return true;
        }
        return point_box
            <
                Point, Box,
                Dimension + 1, DimensionCount
            >::apply(point, box);
    }
};


template <typename Point, typename Box, std::size_t DimensionCount>
struct point_box<Point, Box, DimensionCount, DimensionCount>
{
    static inline bool apply(Point const& , Box const& )
    {
        return false;
    }
};


template
<
    typename Box1, typename Box2,
    std::size_t Dimension, std::size_t DimensionCount
>
struct box_box
{
    static inline bool apply(Box1 const& box1, Box2 const& box2)
    {
        if (get<max_corner, Dimension>(box1) < get<min_corner, Dimension>(box2))
        {
            return true;
        }
        if (get<min_corner, Dimension>(box1) > get<max_corner, Dimension>(box2))
        {
            return true;
        }
        return box_box
            <
                Box1, Box2,
                Dimension + 1, DimensionCount
            >::apply(box1, box2);
    }
};


template <typename Box1, typename Box2, std::size_t DimensionCount>
struct box_box<Box1, Box2, DimensionCount, DimensionCount>
{
    static inline bool apply(Box1 const& , Box2 const& )
    {
        return false;
    }
};


template
<
    typename Geometry1, typename Geometry2
>
struct reverse_covered_by
{
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        return ! geometry::covered_by(geometry1, geometry2);
    }
};



/*!
    \brief Internal utility function to detect of boxes are disjoint
    \note Is used from other algorithms, declared separately
        to avoid circular references
 */
template <typename Box1, typename Box2>
inline bool disjoint_box_box(Box1 const& box1, Box2 const& box2)
{
    return box_box
        <
            Box1, Box2,
            0, dimension<Box1>::type::value
        >::apply(box1, box2);
}



/*!
    \brief Internal utility function to detect of points are disjoint
    \note To avoid circular references
 */
template <typename Point1, typename Point2>
inline bool disjoint_point_point(Point1 const& point1, Point2 const& point2)
{
    return point_point
        <
            Point1, Point2,
            0, dimension<Point1>::type::value
        >::apply(point1, point2);
}


}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace equals
{

/*!
    \brief Internal utility function to detect of points are disjoint
    \note To avoid circular references
 */
template <typename Point1, typename Point2>
inline bool equals_point_point(Point1 const& point1, Point2 const& point2)
{
    return ! detail::disjoint::disjoint_point_point(point1, point2);
}


}} // namespace detail::equals
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_DISJOINT_HPP
