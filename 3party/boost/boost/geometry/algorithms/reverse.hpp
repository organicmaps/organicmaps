// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_REVERSE_HPP
#define BOOST_GEOMETRY_ALGORITHMS_REVERSE_HPP

#include <algorithm>

#include <boost/range.hpp>
#include <boost/typeof/typeof.hpp>

#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace reverse
{


template <typename Range>
struct range_reverse
{
    static inline void apply(Range& range)
    {
        std::reverse(boost::begin(range), boost::end(range));
    }
};


template <typename Polygon>
struct polygon_reverse
{
    static inline void apply(Polygon& polygon)
    {
        typedef typename geometry::ring_type<Polygon>::type ring_type;

        typedef range_reverse<ring_type> per_range;
        per_range::apply(exterior_ring(polygon));

        typename interior_return_type<Polygon>::type rings
                    = interior_rings(polygon);
        for (BOOST_AUTO_TPL(it, boost::begin(rings)); it != boost::end(rings); ++it)
        {
            per_range::apply(*it);
        }
    }
};


}} // namespace detail::reverse
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Tag,
    typename Geometry
>
struct reverse
{
    static inline void apply(Geometry&)
    {}
};


template <typename Ring>
struct reverse<ring_tag, Ring>
    : detail::reverse::range_reverse<Ring>
{};


template <typename LineString>
struct reverse<linestring_tag, LineString>
    : detail::reverse::range_reverse<LineString>
{};


template <typename Polygon>
struct reverse<polygon_tag, Polygon>
    : detail::reverse::polygon_reverse<Polygon>
{};


} // namespace dispatch
#endif


/*!
\brief Reverses the points within a geometry
\details Generic function to reverse a geometry. It resembles the std::reverse
   functionality, but it takes the geometry type into account. Only for a ring
   or for a linestring it is the same as the std::reverse.
\ingroup reverse
\tparam Geometry \tparam_geometry
\param geometry \param_geometry which will be reversed

\qbk{[include reference/algorithms/reverse.qbk]}
*/
template <typename Geometry>
inline void reverse(Geometry& geometry)
{
    concept::check<Geometry>();

    dispatch::reverse
        <
            typename tag<Geometry>::type,
            Geometry
        >::apply(geometry);
}

}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_REVERSE_HPP
