// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_DOMAINS_GIS_IO_HPP
#define BOOST_GEOMETRY_DOMAINS_GIS_IO_HPP


#include <boost/geometry/domains/gis/io/wkt/read_wkt.hpp>
#include <boost/geometry/domains/gis/io/wkt/write_wkt.hpp>

#include <boost/geometry/domains/gis/io/wkt/read_wkt_multi.hpp>
#include <boost/geometry/domains/gis/io/wkt/write_wkt_multi.hpp>

namespace boost { namespace geometry
{

struct format_wkt {};
struct format_wkb {};
struct format_dsv {};

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Tag, typename Geometry>
struct read
{
};

template <typename Geometry>
struct read<format_wkt, Geometry>
{
    static inline void apply(Geometry& geometry, std::string const& wkt)
    {
        read_wkt<typename tag<Geometry>::type, Geometry>::apply(wkt, geometry);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



template <typename Format, typename Geometry>
inline void read(Geometry& geometry, std::string const& wkt)
{
    geometry::concept::check<Geometry>();
    dispatch::read<Format, Geometry>::apply(geometry, wkt);
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_DOMAINS_GIS_IO_HPP
