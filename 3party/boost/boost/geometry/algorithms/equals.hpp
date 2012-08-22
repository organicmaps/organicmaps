// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_EQUALS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_EQUALS_HPP


#include <cstddef>
#include <vector>

#include <boost/mpl/if.hpp>
#include <boost/static_assert.hpp>
#include <boost/range.hpp>

#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/core/reverse_dispatch.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/disjoint.hpp>
#include <boost/geometry/algorithms/detail/not.hpp>

// For trivial checks
#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/select_coordinate_type.hpp>
#include <boost/geometry/util/select_most_precise.hpp>

#include <boost/geometry/algorithms/detail/equals/collect_vectors.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace equals
{


template
<
    typename Box1,
    typename Box2,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct box_box
{
    static inline bool apply(Box1 const& box1, Box2 const& box2)
    {
        if (!geometry::math::equals(get<min_corner, Dimension>(box1), get<min_corner, Dimension>(box2))
            || !geometry::math::equals(get<max_corner, Dimension>(box1), get<max_corner, Dimension>(box2)))
        {
            return false;
        }
        return box_box<Box1, Box2, Dimension + 1, DimensionCount>::apply(box1, box2);
    }
};

template <typename Box1, typename Box2, std::size_t DimensionCount>
struct box_box<Box1, Box2, DimensionCount, DimensionCount>
{
    static inline bool apply(Box1 const& , Box2 const& )
    {
        return true;
    }
};


struct area_check
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        return geometry::math::equals(
                geometry::area(geometry1),
                geometry::area(geometry2));
    }
};


struct length_check
{
    template <typename Geometry1, typename Geometry2>
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        return geometry::math::equals(
                geometry::length(geometry1),
                geometry::length(geometry2));
    }
};


template <typename Geometry1, typename Geometry2, typename TrivialCheck>
struct equals_by_collection
{
    static inline bool apply(Geometry1 const& geometry1, Geometry2 const& geometry2)
    {
        if (! TrivialCheck::apply(geometry1, geometry2))
        {
            return false;
        }

        typedef typename geometry::select_most_precise
            <
                typename select_coordinate_type
                    <
                        Geometry1, Geometry2
                    >::type,
                double
            >::type calculation_type;

        typedef std::vector<collected_vector<calculation_type> > v;
        v c1, c2;

        geometry::collect_vectors(c1, geometry1);
        geometry::collect_vectors(c2, geometry2);

        if (boost::size(c1) != boost::size(c2))
        {
            return false;
        }

        std::sort(c1.begin(), c1.end());
        std::sort(c2.begin(), c2.end());

        // Just check if these vectors are equal.
        return std::equal(c1.begin(), c1.end(), c2.begin());
    }
};


}} // namespace detail::equals
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag1, typename Tag2,
    typename Geometry1,
    typename Geometry2,
    std::size_t DimensionCount
>
struct equals
{};


template <typename P1, typename P2, std::size_t DimensionCount>
struct equals<point_tag, point_tag, P1, P2, DimensionCount>
    : geometry::detail::not_
        <
            P1,
            P2,
            detail::disjoint::point_point<P1, P2, 0, DimensionCount>
        >
{};


template <typename Box1, typename Box2, std::size_t DimensionCount>
struct equals<box_tag, box_tag, Box1, Box2, DimensionCount>
    : detail::equals::box_box<Box1, Box2, 0, DimensionCount>
{};


template <typename Ring1, typename Ring2>
struct equals<ring_tag, ring_tag, Ring1, Ring2, 2>
    : detail::equals::equals_by_collection
        <
            Ring1, Ring2,
            detail::equals::area_check
        >
{};


template <typename Polygon1, typename Polygon2>
struct equals<polygon_tag, polygon_tag, Polygon1, Polygon2, 2>
    : detail::equals::equals_by_collection
        <
            Polygon1, Polygon2,
            detail::equals::area_check
        >
{};


template <typename LineString1, typename LineString2>
struct equals<linestring_tag, linestring_tag, LineString1, LineString2, 2>
    : detail::equals::equals_by_collection
        <
            LineString1, LineString2,
            detail::equals::length_check
        >
{};


template <typename Polygon, typename Ring>
struct equals<polygon_tag, ring_tag, Polygon, Ring, 2>
    : detail::equals::equals_by_collection
        <
            Polygon, Ring,
            detail::equals::area_check
        >
{};


template <typename Ring, typename Box>
struct equals<ring_tag, box_tag, Ring, Box, 2>
    : detail::equals::equals_by_collection
        <
            Ring, Box,
            detail::equals::area_check
        >
{};


template <typename Polygon, typename Box>
struct equals<polygon_tag, box_tag, Polygon, Box, 2>
    : detail::equals::equals_by_collection
        <
            Polygon, Box,
            detail::equals::area_check
        >
{};


template
<
    typename Tag1, typename Tag2,
    typename Geometry1,
    typename Geometry2,
    std::size_t DimensionCount
>
struct equals_reversed
{
    static inline bool apply(Geometry1 const& g1, Geometry2 const& g2)
    {
        return equals
            <
                Tag2, Tag1,
                Geometry2, Geometry1,
                DimensionCount
            >::apply(g2, g1);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_check{are spatially equal}
\details \details_check12{equals, is spatially equal}. Spatially equal means 
    that the same point set is included. A box can therefore be spatially equal
    to a ring or a polygon, or a linestring can be spatially equal to a 
    multi-linestring or a segment. This only theoretically, not all combinations
    are implemented yet.
\ingroup equals
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry
\param geometry2 \param_geometry
\return \return_check2{are spatially equal}

\qbk{[include reference/algorithms/equals.qbk]}

 */
template <typename Geometry1, typename Geometry2>
inline bool equals(Geometry1 const& geometry1, Geometry2 const& geometry2)
{
    concept::check_concepts_and_equal_dimensions
        <
            Geometry1 const,
            Geometry2 const
        >();

    return boost::mpl::if_c
        <
            reverse_dispatch<Geometry1, Geometry2>::type::value,
            dispatch::equals_reversed
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1,
                Geometry2,
                dimension<Geometry1>::type::value
            >,
            dispatch::equals
            <
                typename tag<Geometry1>::type,
                typename tag<Geometry2>::type,
                Geometry1,
                Geometry2,
                dimension<Geometry1>::type::value
            >
        >::type::apply(geometry1, geometry2);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_EQUALS_HPP

