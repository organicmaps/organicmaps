// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_CONVERT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_CONVERT_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/range.hpp>

#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/algorithms/append.hpp>
#include <boost/geometry/algorithms/clear.hpp>
#include <boost/geometry/algorithms/for_each.hpp>
#include <boost/geometry/algorithms/detail/assign_values.hpp>
#include <boost/geometry/algorithms/detail/convert_point_to_point.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace conversion
{

template
<
    typename Point,
    typename Box,
    std::size_t Index,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct point_to_box
{
    static inline void apply(Point const& point, Box& box)
    {
        typedef typename coordinate_type<Box>::type coordinate_type;

        set<Index, Dimension>(box,
                boost::numeric_cast<coordinate_type>(get<Dimension>(point)));
        point_to_box
            <
                Point, Box,
                Index, Dimension + 1, DimensionCount
            >::apply(point, box);
    }
};


template
<
    typename Point,
    typename Box,
    std::size_t Index,
    std::size_t DimensionCount
>
struct point_to_box<Point, Box, Index, DimensionCount, DimensionCount>
{
    static inline void apply(Point const& , Box& )
    {}
};


}} // namespace detail::conversion
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag1, typename Tag2,
    std::size_t DimensionCount,
    typename Geometry1, typename Geometry2
>
struct convert
{
};


template
<
    typename Tag,
    std::size_t DimensionCount,
    typename Geometry1, typename Geometry2
>
struct convert<Tag, Tag, DimensionCount, Geometry1, Geometry2>
{
    // Same geometry type -> copy coordinates from G1 to G2
    // Actually: we try now to just copy it
    static inline void apply(Geometry1 const& source, Geometry2& destination)
    {
        destination = source;
    }
};


template
<
    std::size_t DimensionCount,
    typename Geometry1, typename Geometry2
>
struct convert<point_tag, point_tag, DimensionCount, Geometry1, Geometry2>
    : detail::conversion::point_to_point<Geometry1, Geometry2, 0, DimensionCount>
{};


template <std::size_t DimensionCount, typename Ring1, typename Ring2>
struct convert<ring_tag, ring_tag, DimensionCount, Ring1, Ring2>
{
    static inline void apply(Ring1 const& source, Ring2& destination)
    {
        geometry::clear(destination);
        for (typename boost::range_iterator<Ring1 const>::type it
            = boost::begin(source);
            it != boost::end(source);
            ++it)
        {
            geometry::append(destination, *it);
        }
    }
};


template <typename Box, typename Ring>
struct convert<box_tag, ring_tag, 2, Box, Ring>
{
    static inline void apply(Box const& box, Ring& ring)
    {
        // go from box to ring -> add coordinates in correct order
        geometry::clear(ring);
        typename point_type<Box>::type point;

        geometry::assign_values(point, get<min_corner, 0>(box), get<min_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign_values(point, get<min_corner, 0>(box), get<max_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign_values(point, get<max_corner, 0>(box), get<max_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign_values(point, get<max_corner, 0>(box), get<min_corner, 1>(box));
        geometry::append(ring, point);

        geometry::assign_values(point, get<min_corner, 0>(box), get<min_corner, 1>(box));
        geometry::append(ring, point);
    }
};


template <typename Box, typename Polygon>
struct convert<box_tag, polygon_tag, 2, Box, Polygon>
{
    static inline void apply(Box const& box, Polygon& polygon)
    {
        typedef typename ring_type<Polygon>::type ring_type;

        convert
            <
                box_tag, ring_tag,
                2, Box, ring_type
            >::apply(box, exterior_ring(polygon));
    }
};


template <typename Point, std::size_t DimensionCount, typename Box>
struct convert<point_tag, box_tag, DimensionCount, Point, Box>
{
    static inline void apply(Point const& point, Box& box)
    {
        detail::conversion::point_to_box
            <
                Point, Box, min_corner, 0, DimensionCount
            >::apply(point, box);
        detail::conversion::point_to_box
            <
                Point, Box, max_corner, 0, DimensionCount
            >::apply(point, box);
    }
};


template <typename Ring, std::size_t DimensionCount, typename Polygon>
struct convert<ring_tag, polygon_tag, DimensionCount, Ring, Polygon>
{
    static inline void apply(Ring const& ring, Polygon& polygon)
    {
        typedef typename ring_type<Polygon>::type ring_type;
        convert
            <
                ring_tag, ring_tag, DimensionCount,
                Ring, ring_type
            >::apply(ring, exterior_ring(polygon));
    }
};


template <typename Polygon, std::size_t DimensionCount, typename Ring>
struct convert<polygon_tag, ring_tag, DimensionCount, Polygon, Ring>
{
    static inline void apply(Polygon const& polygon, Ring& ring)
    {
        typedef typename ring_type<Polygon>::type ring_type;

        convert
            <
                ring_tag, ring_tag, DimensionCount,
                ring_type, Ring
            >::apply(exterior_ring(polygon), ring);
    }
};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief Converts one geometry to another geometry
\details The convert algorithm converts one geometry, e.g. a BOX, to another geometry, e.g. a RING. This only
if it is possible and applicable.
\ingroup convert
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry1 \param_geometry (source)
\param geometry2 \param_geometry (target)

\qbk{[include reference/algorithms/convert.qbk]}
 */
template <typename Geometry1, typename Geometry2>
inline void convert(Geometry1 const& geometry1, Geometry2& geometry2)
{
    concept::check_concepts_and_equal_dimensions<Geometry1 const, Geometry2>();

    dispatch::convert
        <
            typename tag<Geometry1>::type,
            typename tag<Geometry2>::type,
            dimension<Geometry1>::type::value,
            Geometry1,
            Geometry2
        >::apply(geometry1, geometry2);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_CONVERT_HPP
