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

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_DISJOINT_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_DISJOINT_HPP

#include <boost/geometry/algorithms/disjoint.hpp>
#include <boost/geometry/algorithms/comparable_distance.hpp>

#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/geometry/extensions/nsphere/views/center_view.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace disjoint
{

// Arvo's algorithm implemented
// TODO - implement the method mentioned in the article below and compare performance
// "On Faster Sphere-Box Overlap Testing" - Larsson, T.; Akeine-Moller, T.; Lengyel, E.
template
<
    typename Box, typename NSphere,
    std::size_t Dimension, std::size_t DimensionCount
>
struct box_nsphere_comparable_distance_cartesian
{
    typedef typename geometry::select_most_precise<
        typename coordinate_type<typename point_type<Box>::type>::type,
        typename coordinate_type<typename point_type<NSphere>::type>::type
    >::type coordinate_type;

    typedef typename geometry::default_distance_result<
        Box,
        typename point_type<NSphere>::type
    >::type result_type;

    static inline result_type apply(Box const& box, NSphere const& nsphere)
    {
        result_type r = 0;

        if( get<Dimension>(nsphere) < get<min_corner, Dimension>(box) )
        {
            coordinate_type tmp = get<min_corner, Dimension>(box) - get<Dimension>(nsphere);
            r = tmp*tmp;
        }
        else if( get<max_corner, Dimension>(box) < get<Dimension>(nsphere) )
        {
            coordinate_type tmp = get<Dimension>(nsphere) - get<max_corner, Dimension>(box);
            r = tmp*tmp;
        }

        return r + box_nsphere_comparable_distance_cartesian<
                        Box, NSphere, Dimension + 1, DimensionCount
                    >::apply(box, nsphere);
    }
};

template <typename Box, typename NSphere, std::size_t DimensionCount>
struct box_nsphere_comparable_distance_cartesian<Box, NSphere, DimensionCount, DimensionCount>
{
    static inline int apply(Box const& , NSphere const& )
    {
        return 0;
    }
};

}} // namespace detail::disjoint
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Point, typename NSphere, std::size_t DimensionCount, bool Reverse>
struct disjoint<Point, NSphere, DimensionCount, point_tag, nsphere_tag, Reverse>
{
    static inline bool apply(Point const& p, NSphere const& s)
    {
        typedef typename coordinate_system<Point>::type p_cs;
        typedef typename coordinate_system<NSphere>::type s_cs;
        static const bool check_cs = ::boost::is_same<p_cs, cs::cartesian>::value && ::boost::is_same<s_cs, cs::cartesian>::value;
        BOOST_MPL_ASSERT_MSG(check_cs, NOT_IMPLEMENTED_FOR_THOSE_COORDINATE_SYSTEMS, (p_cs, s_cs));

        return get_radius<0>(s) * get_radius<0>(s)
               <   geometry::comparable_distance(p, center_view<const NSphere>(s));
    }
};

template <typename NSphere, typename Box, std::size_t DimensionCount, bool Reverse>
struct disjoint<NSphere, Box, DimensionCount, nsphere_tag, box_tag, Reverse>
{
    static inline bool apply(NSphere const& s, Box const& b)
    {
        typedef typename coordinate_system<Box>::type b_cs;
        typedef typename coordinate_system<NSphere>::type s_cs;
        static const bool check_cs = ::boost::is_same<b_cs, cs::cartesian>::value && ::boost::is_same<s_cs, cs::cartesian>::value;
        BOOST_MPL_ASSERT_MSG(check_cs, NOT_IMPLEMENTED_FOR_THOSE_COORDINATE_SYSTEMS, (b_cs, s_cs));

        return get_radius<0>(s) * get_radius<0>(s)
               <   geometry::detail::disjoint::box_nsphere_comparable_distance_cartesian<
                       Box, NSphere, 0, DimensionCount
                   >::apply(b, s);
    }
};

template <typename NSphere1, typename NSphere2, std::size_t DimensionCount, bool Reverse>
struct disjoint<NSphere1, NSphere2, DimensionCount, nsphere_tag, nsphere_tag, Reverse>
{
    static inline bool apply(NSphere1 const& s1, NSphere2 const& s2)
    {
        typedef typename coordinate_system<NSphere1>::type s1_cs;
        typedef typename coordinate_system<NSphere2>::type s2_cs;
        static const bool check_cs = ::boost::is_same<s1_cs, cs::cartesian>::value && ::boost::is_same<s2_cs, cs::cartesian>::value;
        BOOST_MPL_ASSERT_MSG(check_cs, NOT_IMPLEMENTED_FOR_THOSE_COORDINATE_SYSTEMS, (s1_cs, s2_cs));

        /*return get_radius<0>(s1) + get_radius<0>(s2)
               <   ::sqrt(geometry::comparable_distance(center_view<NSphere>(s1), center_view<NSphere>(s2)));*/

        return get_radius<0>(s1) * get_radius<0>(s1) + 2 * get_radius<0>(s1) * get_radius<0>(s2) + get_radius<0>(s2) * get_radius<0>(s2)
               <   geometry::comparable_distance(center_view<const NSphere1>(s1), center_view<const NSphere2>(s2));
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_DISJOINT_HPP
