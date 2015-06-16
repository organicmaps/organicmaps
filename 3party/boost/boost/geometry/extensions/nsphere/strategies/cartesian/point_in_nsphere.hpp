// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_STRATEGIES_CARTESIAN_POINT_IN_NSPHERE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_STRATEGIES_CARTESIAN_POINT_IN_NSPHERE_HPP

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>

#include <boost/geometry/extensions/nsphere/views/center_view.hpp>

namespace boost { namespace geometry { namespace strategy
{

namespace within
{

struct point_nsphere_within_comparable_distance
{
    template <typename ComparableDistance, typename Radius>
    static inline bool apply(ComparableDistance const& ed_comp_dist
                           , Radius const& ing_radius)
    {
        return ed_comp_dist < ing_radius * ing_radius;
    }
};


struct point_nsphere_covered_by_comparable_distance
{
    template <typename ComparableDistance, typename Radius>
    static inline bool apply(ComparableDistance const& ed_comp_dist
                           , Radius const& ing_radius)
    {
        return ed_comp_dist <= ing_radius * ing_radius;
    }
};

template
<
    typename Point,
    typename NSphere,
    typename SubStrategy = point_nsphere_within_comparable_distance
>
struct point_in_nsphere
{
    static inline bool apply(Point const& point, NSphere const& nsphere)
    {
        return SubStrategy::apply(
                    geometry::comparable_distance(point, center_view<NSphere const>(nsphere)),
                    get_radius<0>(nsphere));
    }
};


//// For many geometry-in-nsphere, we do not have a strategy yet... but a default strategy should exist
//struct nsphere_dummy
//{
//    template <typename A, typename B>
//    static bool apply(A const& a, B const& b)
//    {
//        // Assertion if called
//        BOOST_GEOMETRY_ASSERT(false);
//        return false;
//    }
//};



} // namespace within


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


namespace within { namespace services
{

template <typename Point, typename NSphere>
struct default_strategy
    <
        point_tag, nsphere_tag,
        point_tag, nsphere_tag,
        cartesian_tag, cartesian_tag,
        Point, NSphere
    >
{
    typedef within::point_in_nsphere<Point, NSphere, within::point_nsphere_within_comparable_distance> type;
};

//template <typename AnyTag, typename AnyGeometry, typename NSphere>
//struct default_strategy
//    <
//        AnyTag, nsphere_tag,
//        AnyTag, nsphere_tag,
//        cartesian_tag, cartesian_tag,
//        AnyGeometry, NSphere
//    >
//{
//    typedef within::nsphere_dummy type;
//};


}} // namespace within::services


namespace covered_by { namespace services
{


template <typename Point, typename NSphere>
struct default_strategy
    <
        point_tag, nsphere_tag,
        point_tag, nsphere_tag,
        cartesian_tag, cartesian_tag,
        Point, NSphere
    >
{
    typedef within::point_in_nsphere<Point, NSphere, within::point_nsphere_covered_by_comparable_distance> type;
};


}} // namespace covered_by::services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}}} // namespace boost::geometry::strategy


#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_STRATEGIES_CARTESIAN_NSPHERE_IN_BOX_HPP
