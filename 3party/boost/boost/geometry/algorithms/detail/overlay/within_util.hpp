// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_WITHIN_UTIL_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_WITHIN_UTIL_HPP



#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/strategies/agnostic/point_in_poly_winding.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay
{


template<typename Tag, typename Point, typename Geometry>
struct within_code
{};

template<typename Point, typename Box>
struct within_code<box_tag, Point, Box>
{
    static inline int apply(Point const& point, Box const& box)
    {
        // 1. Check outside
        if (get<0>(point) < get<min_corner, 0>(box)
            || get<0>(point) > get<max_corner, 0>(box)
            || get<1>(point) < get<min_corner, 1>(box)
            || get<1>(point) > get<max_corner, 1>(box))
        {
            return -1;
        }
        // 2. Check border
        if (geometry::math::equals(get<0>(point), get<min_corner, 0>(box))
            || geometry::math::equals(get<0>(point), get<max_corner, 0>(box))
            || geometry::math::equals(get<1>(point), get<min_corner, 1>(box))
            || geometry::math::equals(get<1>(point), get<max_corner, 1>(box)))
        {
            return 0;
        }
        return 1;
    }
};
template<typename Point, typename Ring>
struct within_code<ring_tag, Point, Ring>
{
    static inline int apply(Point const& point, Ring const& ring)
    {
        // Same as point_in_ring but here ALWAYS with winding.
        typedef strategy::within::winding<Point> strategy_type;

        return detail::within::point_in_ring
            <
                Point,
                Ring,
                order_as_direction<geometry::point_order<Ring>::value>::value,
                geometry::closure<Ring>::value,
                strategy_type
            >::apply(point, ring, strategy_type());
    }
};


template<typename Point, typename Geometry>
inline int point_in_ring(Point const& point, Geometry const& geometry)
{
    return within_code<typename geometry::tag<Geometry>::type, Point, Geometry>
        ::apply(point, geometry);
}

template<typename Point, typename Geometry>
inline bool within_or_touch(Point const& point, Geometry const& geometry)
{
    return within_code<typename geometry::tag<Geometry>::type, Point, Geometry>
        ::apply(point, geometry) >= 0;
}



}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_WITHIN_UTIL_HPP
