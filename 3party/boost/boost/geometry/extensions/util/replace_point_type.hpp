// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_GEOMETRY_EXTENSIONS_UTIL_REPLACE_POINT_TYPE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_UTIL_REPLACE_POINT_TYPE_HPP


#include <boost/type_traits/remove_const.hpp>


#include <boost/geometry/core/tag.hpp>
#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/coordinate_type.hpp>

// For now: use ggl-provided geometries
// TODO: figure out how to get the class and replace the type
// TODO: take "const" into account
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/geometries/box.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{
template <typename GeometryTag, typename Geometry, typename NewPointType>
struct replace_point_type {};

template <typename Geometry, typename NewPointType>
struct replace_point_type<point_tag, Geometry, NewPointType>
{
    typedef NewPointType type;
};

template <typename Geometry, typename NewPointType>
struct replace_point_type<linestring_tag, Geometry, NewPointType>
{
    typedef model::linestring<NewPointType> type;
};

template <typename Geometry, typename NewPointType>
struct replace_point_type<segment_tag, Geometry, NewPointType>
{
    typedef model::segment<NewPointType> type;
};

template <typename Geometry, typename NewPointType>
struct replace_point_type<ring_tag, Geometry, NewPointType>
{
    typedef model::ring<NewPointType> type;
};

template <typename Geometry, typename NewPointType>
struct replace_point_type<box_tag, Geometry, NewPointType>
{
    typedef model::box<NewPointType> type;
};

template <typename Geometry, typename NewPointType>
struct replace_point_type<polygon_tag, Geometry, NewPointType>
{
    typedef model::polygon<NewPointType> type;
};


} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


template <typename Geometry, typename NewPointType>
struct replace_point_type : core_dispatch::replace_point_type
        <
            typename tag<Geometry>::type,
            typename boost::remove_const<Geometry>::type,
            NewPointType
        >
{};

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_UTIL_REPLACE_POINT_TYPE_HPP
