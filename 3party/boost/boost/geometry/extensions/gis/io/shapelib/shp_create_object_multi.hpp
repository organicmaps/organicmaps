// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_CREATE_OBJECT_MULTI_HPP
#define BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_CREATE_OBJECT_MULTI_HPP


#include <boost/range.hpp>

#include <boost/scoped_array.hpp>

#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/multi/algorithms/num_interior_rings.hpp>

#include <boost/geometry/extensions/gis/io/shapelib/shp_create_object.hpp>




namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace shp_create_object
{


template <typename MultiPoint>
struct shape_create_multi_point
{
    static inline SHPObject* apply(MultiPoint const& multi)
    {
        int const n = boost::size(multi);
        boost::scoped_array<double> x(new double[n]);
        boost::scoped_array<double> y(new double[n]);

        range_to_part(multi, x.get(), y.get());

        int const parts = 0;
        return ::SHPCreateObject(SHPT_MULTIPOINT, -1, 1, &parts, NULL,
                                    n, x.get(), y.get(), NULL, NULL);
    }
};



template <typename MultiLinestring>
struct shape_create_multi_linestring
{
    static inline SHPObject* apply(MultiLinestring const& multi)
    {
        int const n = geometry::num_points(multi);
        int const part_count = boost::size(multi);

        boost::scoped_array<double> x(new double[n]);
        boost::scoped_array<double> y(new double[n]);
        boost::scoped_array<int> parts(new int[part_count]);

        int ring = 0;
        int offset = 0;

        for (typename boost::range_iterator<MultiLinestring const>::type
                    it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            parts[ring++] = offset;
            offset = range_to_part(*it, x.get(), y.get(), offset);
        }

        return ::SHPCreateObject(SHPT_ARC, -1, part_count, parts.get(), NULL,
                                    n, x.get(), y.get(), NULL, NULL);
    }
};


template <typename MultiPolygon>
struct shape_create_multi_polygon
{
    static inline SHPObject* apply(MultiPolygon const& multi)
    {
        int const n = geometry::num_points(multi);
        int const ring_count = boost::size(multi) + geometry::num_interior_rings(multi);

        boost::scoped_array<double> x(new double[n]);
        boost::scoped_array<double> y(new double[n]);
        boost::scoped_array<int> parts(new int[ring_count]);

        int ring = 0;
        int offset = 0;

        typedef typename boost::range_value<MultiPolygon>::type polygon_type;
        for (typename boost::range_iterator<MultiPolygon const>::type
                    it = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            shape_create_polygon<polygon_type>::process_polygon(*it, x.get(), y.get(), parts.get(),
                offset, ring);
        }

        return ::SHPCreateObject(SHPT_POLYGON, -1, ring_count, parts.get(), NULL,
                                    n, x.get(), y.get(), NULL, NULL);
    }
};


}} // namespace detail::shp_create_object
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename MultiPoint>
struct shp_create_object<multi_point_tag, MultiPoint>
    : detail::shp_create_object::shape_create_multi_point<MultiPoint>
{};


template <typename MultiLinestring>
struct shp_create_object<multi_linestring_tag, MultiLinestring>
    : detail::shp_create_object::shape_create_multi_linestring<MultiLinestring>
{};


template <typename MultiPolygon>
struct shp_create_object<multi_polygon_tag, MultiPolygon>
    : detail::shp_create_object::shape_create_multi_polygon<MultiPolygon>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_CREATE_OBJECT_MULTI_HPP
