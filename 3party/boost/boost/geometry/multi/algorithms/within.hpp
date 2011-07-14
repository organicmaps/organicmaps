// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_WITHIN_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_WITHIN_HPP


#include <boost/range.hpp>

#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/multi/core/closure.hpp>
#include <boost/geometry/multi/core/point_order.hpp>
#include <boost/geometry/multi/core/tags.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace within
{


template
<
    typename Geometry,
    typename MultiGeometry,
    typename Strategy,
    typename Policy
>
struct geometry_in_multi
{
    static inline int apply(Geometry const& geometry,
            MultiGeometry const& multi,
            Strategy const& strategy)
    {
        for (typename boost::range_iterator<MultiGeometry const>::type it
                    = boost::begin(multi);
            it != boost::end(multi);
            ++it)
        {
            // Geometry within a multi: true if within one of them
            int const code = Policy::apply(geometry, *it, strategy);
            if (code != -1)
            {
                return code;
            }
        }
        return -1;
    }
};


}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Point, typename MultiPolygon, typename Strategy>
struct within<point_tag, multi_polygon_tag, Point, MultiPolygon, Strategy>
    : detail::within::geometry_in_multi
        <
            Point,
            MultiPolygon,
            Strategy,
            detail::within::point_in_polygon
                    <
                        Point,
                        typename boost::range_value<MultiPolygon>::type,
                        order_as_direction<geometry::point_order<MultiPolygon>::value>::value,
                        geometry::closure<MultiPolygon>::value,
                        Strategy
                    >
        >
{};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_WITHIN_HPP
