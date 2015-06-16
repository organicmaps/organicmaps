// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_MIDPOINTS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_MIDPOINTS_HPP

// Renamed from "intermediate" to "midpoints"

#include <cstddef>
#include <iterator>

#include <boost/range.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace midpoints {

template <typename Src, typename Dst, std::size_t Dimension, std::size_t DimensionCount>
struct calculate_coordinate
{
    static inline void apply(Src const& p1, Src const& p2, Dst& p)
    {
        geometry::set<Dimension>(p,
                    (geometry::get<Dimension>(p1) + geometry::get<Dimension>(p2)) / 2.0);
        calculate_coordinate<Src, Dst, Dimension + 1, DimensionCount>::apply(p1, p2, p);
    }
};

template <typename Src, typename Dst, std::size_t DimensionCount>
struct calculate_coordinate<Src, Dst, DimensionCount, DimensionCount>
{
    static inline void apply(Src const&, Src const&, Dst&)
    {
    }
};

template<typename Range, typename Iterator>
struct range_midpoints
{
    static inline void apply(Range const& range,
            bool start_and_end, Iterator out)
    {
        typedef typename point_type<Range>::type point_type;
        typedef typename boost::range_iterator<Range const>::type iterator_type;

        iterator_type it = boost::begin(range);

        if (start_and_end)
        {
            *out++ = *it;
        }

        iterator_type prev = it++;
        for (; it != boost::end(range); prev = it++)
        {
            point_type p;
            calculate_coordinate
                <
                    point_type,
                    point_type,
                    0,
                    dimension<point_type>::type::value
                >::apply(*prev, *it, p);
            *out++ = p;
        }

        if (start_and_end)
        {
            *out++ = *prev;
        }
    }
};

}} // namespace detail::midpoints
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename GeometryTag, typename G, typename Iterator>
struct midpoints  {};

template <typename G, typename Iterator>
struct midpoints<ring_tag, G, Iterator>
        : detail::midpoints::range_midpoints<G, Iterator> {};

template <typename G, typename Iterator>
struct midpoints<linestring_tag, G, Iterator>
        : detail::midpoints::range_midpoints<G, Iterator> {};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
    \brief Calculate midpoints of a geometry
    \ingroup midpoints
 */
template<typename Geometry, typename Iterator>
inline void midpoints(Geometry const& geometry,
        bool start_and_end, Iterator out)
{
    concept::check<Geometry const>();

    dispatch::midpoints
        <
            typename tag<Geometry>::type,
            Geometry,
            Iterator
        >::apply(geometry, start_and_end, out);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGORITHMS_MIDPOINTS_HPP
