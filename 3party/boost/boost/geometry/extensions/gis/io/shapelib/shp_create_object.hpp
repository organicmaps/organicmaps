// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_CREATE_OBJECT_HPP
#define BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_CREATE_OBJECT_HPP


#include <boost/mpl/assert.hpp>
#include <boost/range.hpp>
#include <boost/scoped_array.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/algorithms/num_interior_rings.hpp>
#include <boost/geometry/algorithms/num_points.hpp>
#include <boost/geometry/views/box_view.hpp>
#include <boost/geometry/views/segment_view.hpp>


// Should be somewhere in your include path
#include "shapefil.h"



namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace shp_create_object
{

template <typename Point>
struct shape_create_point
{

    static inline SHPObject* apply(Point const& point)
    {
        double x = get<0>(point);
        double y = get<1>(point);

        int const parts = 0;

        return ::SHPCreateObject(SHPT_POINT, -1, 1, &parts, NULL,
                                    1, &x, &y, NULL, NULL);
    }
};


template <typename Range>
static inline int range_to_part(Range const& range, double* x, double* y, int offset = 0)
{
    x += offset;
    y += offset;

    for (typename boost::range_iterator<Range const>::type
        it = boost::begin(range);
        it != boost::end(range);
        ++it, ++x, ++y)
    {
        *x = get<0>(*it);
        *y = get<1>(*it);
        offset++;
    }
    return offset;
}


template <typename Range, int ShapeType>
struct shape_create_range
{
    static inline SHPObject* apply(Range const& range)
    {
        int const n = boost::size(range);

        boost::scoped_array<double> x(new double[n]);
        boost::scoped_array<double> y(new double[n]);

        range_to_part(range, x.get(), y.get());

        int const parts = 0;

        return ::SHPCreateObject(ShapeType, -1, 1, &parts, NULL,
                                    n, x.get(), y.get(), NULL, NULL);
    }
};


template <typename Polygon>
struct shape_create_polygon
{
    static inline void process_polygon(Polygon const& polygon,
            double* xp, double* yp, int* parts,
            int& offset, int& ring)
    {
        parts[ring++] = offset;
        offset = range_to_part(geometry::exterior_ring(polygon), xp, yp, offset);

        typename interior_return_type<Polygon const>::type rings
                    = interior_rings(polygon);

        typedef typename boost::range_const_iterator
            <
                typename interior_type<Polygon const>::type
            >::type ring_iterator;

        for (ring_iterator it = boost::begin(rings); it != boost::end(rings); ++it)
        {
            parts[ring++] = offset;
            offset = range_to_part(*it, xp, yp, offset);
        }
    }

    static inline SHPObject* apply(Polygon const& polygon)
    {
        int const n = geometry::num_points(polygon);
        int const ring_count = 1 + geometry::num_interior_rings(polygon);

        boost::scoped_array<double> x(new double[n]);
        boost::scoped_array<double> y(new double[n]);
        boost::scoped_array<int> parts(new int[ring_count]);

        int ring = 0;
        int offset = 0;

        process_polygon(polygon, x.get(), y.get(), parts.get(), offset, ring);

        return ::SHPCreateObject(SHPT_POLYGON, -1, ring_count, parts.get(), NULL,
                                    n, x.get(), y.get(), NULL, NULL);
    }
};

template <typename Geometry, typename AdaptedRange, int ShapeType>
struct shape_create_adapted_range
{
    static inline SHPObject* apply(Geometry const& geometry)
    {
        return shape_create_range<AdaptedRange, ShapeType>::apply(AdaptedRange(geometry));
    }
};



}} // namespace detail::shp_create_object
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Tag, typename Geometry>
struct shp_create_object
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (Geometry)
        );
};


template <typename Point>
struct shp_create_object<point_tag, Point>
    : detail::shp_create_object::shape_create_point<Point>
{};


template <typename LineString>
struct shp_create_object<linestring_tag, LineString>
    : detail::shp_create_object::shape_create_range<LineString, SHPT_ARC>
{};


template <typename Ring>
struct shp_create_object<ring_tag, Ring>
    : detail::shp_create_object::shape_create_range<Ring, SHPT_POLYGON>
{};


template <typename Polygon>
struct shp_create_object<polygon_tag, Polygon>
    : detail::shp_create_object::shape_create_polygon<Polygon>
{};

template <typename Box>
struct shp_create_object<box_tag, Box>
    : detail::shp_create_object::shape_create_adapted_range
        <
            Box,
            box_view<Box>,
            SHPT_POLYGON
        >
{};

template <typename Segment>
struct shp_create_object<segment_tag, Segment>
    : detail::shp_create_object::shape_create_adapted_range
        <
            Segment,
            segment_view<Segment>,
            SHPT_ARC
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


// Redirect shapelib's SHPCreateObject to this boost::geometry::SHPCreateObject.
// The only difference is their parameters, one just accepts a geometry
template <typename Geometry>
inline SHPObject* SHPCreateObject(Geometry const& geometry)
{
    return dispatch::shp_create_object
        <
            typename tag<Geometry>::type, Geometry
        >::apply(geometry);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_CREATE_OBJECT_HPP
