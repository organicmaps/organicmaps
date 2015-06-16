// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_READ_OBJECT_HPP
#define BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_READ_OBJECT_HPP


#include <boost/mpl/assert.hpp>
#include <boost/range.hpp>
#include <boost/scoped_array.hpp>

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/ring_type.hpp>
#include <boost/geometry/algorithms/num_points.hpp>


// Should be somewhere in your include path
#include "shapefil.h"



namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace shp_read_object
{


template <typename Pair>
struct sort_on_area_desc
{
    inline bool operator()(Pair const& left, Pair const& right)
    {
        return left.second > right.second;
    }
};



template <typename LineString>
struct read_linestring
{
    static inline SHPObject* apply(LineString const& linestring)
    {
        typedef typename geometry::point_type<Linestring>::type point_type;

        if (shape.nSHPType == SHPT_ARCZ && shape.nParts == 1)
        {
            double* const x = shape.padfX;
            double* const y = shape.padfY;

            for (int i = 0; i < shape.nVertices; i++)
            {
                point_type point;
                geometry::set<0>(point, x[i]);
                geometry::set<1>(point, y[i]);

                linestring.push_back(point);
            }
            return true;
        }
        return false;

    }
};


template <typename Polygon>
struct read_polygon
{
    static inline SHPObject* apply(Polygon const& polygon)
    {
        typedef typename geometry::point_type<Polygon>::type point_type;
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        if (shape.nSHPType == SHPT_POLYGON)
        {
            //std::cout << shape.nParts << " " << shape.nVertices << std::endl;

            double* const x = shape.padfX;
            double* const y = shape.padfY;

            typedef std::pair<ring_type, double> ring_plus_area;
            std::vector<ring_plus_area> rings;
            rings.resize(shape.nParts);

            int v = 0;
            for (int p = 0; p < shape.nParts; p++)
            {
                int const first = shape.panPartStart[p];
                int const last = p + 1 < shape.nParts
                    ? shape.panPartStart[p + 1]
                    : shape.nVertices;

                for (v = first; v < last; v++)
                {
                    point_type point;
                    geometry::set<0>(point, x[v]);
                    geometry::set<1>(point, y[v]);
                    rings[p].first.push_back(point);
                }
                rings[p].second = geometry::math::abs(geometry::area(rings[p].first));
            }

            if (rings.size() > 1)
            {
                // Sort rings on area
                std::sort(rings.begin(), rings.end(),
                        sort_on_area_desc<ring_plus_area>());
                // Largest area (either positive or negative) is outer ring
                // Rest of the rings are holes
                geometry::exterior_ring(polygon) = rings.front().first;
                for (int i = 1; i < rings.size(); i++)
                {
                    geometry::interior_rings(polygon).push_back(rings[i].first);
                    if (! geometry::within(rings[i].first.front(), geometry::exterior_ring(polygon))
                        && ! geometry::within(rings[i].first.at(1), geometry::exterior_ring(polygon))
                        )
                    {
    #if ! defined(NDEBUG)
                        std::cout << "Error: inconsistent ring!" << std::endl;
                        BOOST_FOREACH(ring_plus_area const& r, rings)
                        {
                            std::cout << geometry::area(r.first) << " "
                                << geometry::wkt(r.first.front()) << " "
                                << std::endl;
                        }
    #endif
                    }
                }
            }
            else if (rings.size() == 1)
            {
                geometry::exterior_ring(polygon) = rings.front().first;
            }
            return true;
        }
        return false;
    }
};



}} // namespace detail::shp_read_object
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Tag, typename Geometry>
struct shp_read_object
{
    BOOST_MPL_ASSERT_MSG
        (
            false, NOT_OR_NOT_YET_IMPLEMENTED_FOR_THIS_GEOMETRY_TYPE
            , (Geometry)
        );
};


template <typename LineString>
struct shp_read_object<linestring_tag, LineString>
    : detail::shp_read_object::read_linestring<LineString>
{};




template <typename Polygon>
struct shp_read_object<polygon_tag, Polygon>
    : detail::shp_read_object::read_polygon<Polygon>
{};



} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template <typename Geometry>
inline void read_shapefile(std::string const& filename,
                    std::vector<Geometry>& geometries)
{

    try
    {
        // create shape_reader

        for (int i = 0; i < shape_reader.Count(); i++)
        {
            SHPObject* psShape = SHPReadObject(shp_handle, i);
            Geometry geometry;
            if (dispatch::shp_read_object<Geometry>(*psShape, geometry))
            {
                geometries.push_back(geometry);
            }
            SHPDestroyObject( psShape );
        }

    }
    catch(std::string s)
    {
        throw s;
    }
    catch(...)
    {
        throw std::string("Other exception");
    }
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXT_GIS_IO_SHAPELIB_SHP_READ_OBJECT_HPP
