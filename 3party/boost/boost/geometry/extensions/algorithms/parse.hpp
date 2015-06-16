// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_PARSE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_PARSE_HPP

#include <string>


#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/extensions/gis/geographic/strategies/dms_parser.hpp>
#include <boost/geometry/extensions/strategies/parse.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{

} // namespace detail
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{
template <typename Tag, typename G>
struct parsing
{
};

template <typename Point>
struct parsing<point_tag, Point>
{
    template <typename S>
    static inline void parse(Point& point, std::string const& c1, std::string const& c2, S const& strategy)
    {
        assert_dimension<Point, 2>();
        dms_result r1 = strategy(c1.c_str());
        dms_result r2 = strategy(c2.c_str());

        if (0 == r1.axis())
            set<0>(point, r1);
        else
            set<1>(point, r1);

        if (0 == r2.axis())
            set<0>(point, r2);
        else
            set<1>(point, r2);
    }

    static inline void parse(Point& point, std::string const& c1, std::string const& c2)
    {
        // strategy-parser corresponding to degree/radian
        typename strategy_parse
            <
            typename cs_tag<Point>::type,
            typename coordinate_system<Point>::type
            >::type strategy;

        parse(point, c1, c2, strategy);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief parse two strings to a spherical/geographic point, using W/E/N/S
    \ingroup parse
 */
template <typename Geometry>
inline void parse(Geometry& geometry, std::string const& c1, std::string const& c2)
{
    concept::check<Geometry>();
    dispatch::parsing<typename tag<Geometry>::type, Geometry>::parse(geometry, c1, c2);
}

/*!
    \brief parse two strings to a spherical/geographic point, using a specified strategy
    \details user can use N/E/S/O or N/O/Z/W or other formats
    \ingroup parse
 */
template <typename Geometry, typename S>
inline void parse(Geometry& geometry, std::string const& c1,
        std::string const& c2, S const& strategy)
{
    concept::check<Geometry>();
    dispatch::parsing<typename tag<Geometry>::type, Geometry>::parse(geometry, c1, c2, strategy);
}

// There will be a parsing function with three arguments (ANGLE,ANGLE,RADIUS)

template <typename Geometry>
inline Geometry parse(std::string const& c1, std::string const& c2)
{
    concept::check<Geometry>();

    Geometry geometry;
    dispatch::parsing<typename tag<Geometry>::type, Geometry>::parse(geometry, c1, c2);
    return geometry;
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_PARSE_HPP
