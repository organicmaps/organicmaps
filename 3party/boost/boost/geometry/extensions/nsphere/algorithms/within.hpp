// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_WITHIN_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_WITHIN_HPP


#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/make.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/strategies/distance.hpp>

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/extensions/nsphere/core/access.hpp>
#include <boost/geometry/extensions/nsphere/core/radius.hpp>
#include <boost/geometry/extensions/nsphere/core/tags.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/assign.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace within
{



//-------------------------------------------------------------------------------------------------------
// Implementation for n-spheres. Supports circles or spheres, in 2 or 3 dimensions, in Euclidian system
// Circle center might be of other point-type as geometry
// Todo: implement as strategy
//-------------------------------------------------------------------------------------------------------
template<typename P, typename C>
inline bool point_in_circle(P const& p, C const& c)
{
    namespace services = strategy::distance::services;

    assert_dimension<C, 2>();

    typedef typename point_type<C>::type point_type;
    typedef typename services::default_strategy
        <
            point_tag, point_tag, P, point_type
        >::type strategy_type;
    typedef typename services::return_type<strategy_type, P, point_type>::type return_type;

    strategy_type strategy;

    P const center = geometry::make<P>(get<0>(c), get<1>(c));
    return_type const r = geometry::distance(p, center, strategy);
    return_type const rad = services::result_from_distance
        <
            strategy_type, P, point_type
        >::apply(strategy, get_radius<0>(c));

    return r < rad;
}
/// 2D version
template<typename T, typename C>
inline bool point_in_circle(T const& c1, T const& c2, C const& c)
{
    typedef typename point_type<C>::type point_type;

    point_type p = geometry::make<point_type>(c1, c2);
    return point_in_circle(p, c);
}

template<typename B, typename C>
inline bool box_in_circle(B const& b, C const& c)
{
    typedef typename point_type<B>::type point_type;

    // Currently only implemented for 2d geometries
    assert_dimension<point_type, 2>();
    assert_dimension<C, 2>();

    // Box: all four points must lie within circle

    // Check points lower-left and upper-right, then lower-right and upper-left
    return point_in_circle(get<min_corner, 0>(b), get<min_corner, 1>(b), c)
        && point_in_circle(get<max_corner, 0>(b), get<max_corner, 1>(b), c)
        && point_in_circle(get<min_corner, 0>(b), get<max_corner, 1>(b), c)
        && point_in_circle(get<max_corner, 0>(b), get<min_corner, 1>(b), c);
}

// Generic "range-in-circle", true if all points within circle
template<typename R, typename C>
inline bool range_in_circle(R const& range, C const& c)
{
    assert_dimension<R, 2>();
    assert_dimension<C, 2>();

    for (typename boost::range_iterator<R const>::type it = boost::begin(range);
         it != boost::end(range); ++it)
    {
        if (! point_in_circle(*it, c))
        {
            return false;
        }
    }

    return true;
}

template<typename Y, typename C>
inline bool polygon_in_circle(Y const& poly, C const& c)
{
    return range_in_circle(exterior_ring(poly), c);
}



template<typename I, typename C>
inline bool multi_polygon_in_circle(I const& m, C const& c)
{
    for (typename I::const_iterator i = m.begin(); i != m.end(); i++)
    {
        if (! polygon_in_circle(*i, c))
        {
            return false;
        }
    }
    return true;
}



}} // namespace detail::within
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename P, typename Circle>
struct within<P, Circle, point_tag, nsphere_tag>
{
    template <typename Strategy>
    static inline bool apply(P const& p, Circle const& c, Strategy const& strategy)
    {
        ::boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(p, c);
        //return detail::within::point_in_circle(p, c);
    }
};

template <typename Box, typename Circle>
struct within<Box, Circle, box_tag, nsphere_tag>
{
    template <typename Strategy>
    static inline bool apply(Box const& b, Circle const& c, Strategy const&)
    {
        return detail::within::box_in_circle(b, c);
    }
};

template <typename Linestring, typename Circle>
struct within<Linestring, Circle, linestring_tag, nsphere_tag>
{
    template <typename Strategy>
    static inline bool apply(Linestring const& ln, Circle const& c, Strategy const&)
    {
        return detail::within::range_in_circle(ln, c);
    }
};

template <typename Ring, typename Circle>
struct within<Ring, Circle, ring_tag, nsphere_tag>
{
    template <typename Strategy>
    static inline bool apply(Ring const& r, Circle const& c, Strategy const&)
    {
        return detail::within::range_in_circle(r, c);
    }
};

template <typename Polygon, typename Circle>
struct within<Polygon, Circle, polygon_tag, nsphere_tag>
{
    template <typename Strategy>
    static inline bool apply(Polygon const& poly, Circle const& c, Strategy const&)
    {
        return detail::within::polygon_in_circle(poly, c);
    }
};

template <typename M, typename C>
struct within<M, C, multi_polygon_tag, nsphere_tag>
{
    template <typename Strategy>
    static inline bool apply(M const& m, C const& c, Strategy const&)
    {
        return detail::within::multi_polygon_in_circle(m, c);
    }
};



template <typename NSphere, typename Box>
struct within<NSphere, Box, nsphere_tag, box_tag>
{
    template <typename Strategy>
    static inline bool apply(NSphere const& nsphere, Box const& box, Strategy const& strategy)
    {
        assert_dimension_equal<NSphere, Box>();
        boost::ignore_unused_variable_warning(strategy);
        return strategy.apply(nsphere, box);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_WITHIN_HPP
