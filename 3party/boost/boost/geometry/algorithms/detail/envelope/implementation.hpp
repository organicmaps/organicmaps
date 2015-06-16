// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2015.
// Modifications copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_IMPLEMENTATION_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_IMPLEMENTATION_HPP

#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/algorithms/detail/envelope/box.hpp>
#include <boost/geometry/algorithms/detail/envelope/linestring.hpp>
#include <boost/geometry/algorithms/detail/envelope/multilinestring.hpp>
#include <boost/geometry/algorithms/detail/envelope/multipoint.hpp>
#include <boost/geometry/algorithms/detail/envelope/point.hpp>
#include <boost/geometry/algorithms/detail/envelope/range.hpp>
#include <boost/geometry/algorithms/detail/envelope/segment.hpp>

#include <boost/geometry/algorithms/dispatch/envelope.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


struct exterior_ring_expand_policy
{
    template <typename Box, typename Geometry>
    static inline void apply(Box& mbr, Geometry const& geometry)
    {
        Box ring_mbr;
        envelope_range<>::apply(exterior_ring(geometry), ring_mbr);
        geometry::expand(mbr, ring_mbr);
    }
};


struct envelope_polygon
{
    template <typename Polygon, typename Box>
    static inline void apply(Polygon const& poly, Box& mbr)
    {
        // For polygon, inspecting outer ring is sufficient
        envelope_range<>::apply(exterior_ring(poly), mbr);
    }

};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Ring>
struct envelope<Ring, ring_tag>
    : detail::envelope::envelope_range<>
{};


template <typename Polygon>
struct envelope<Polygon, polygon_tag>
    : detail::envelope::envelope_polygon
{};


template <typename MultiPolygon>
struct envelope<MultiPolygon, multi_polygon_tag>
    : detail::envelope::envelope_range
        <
            detail::envelope::exterior_ring_expand_policy
        >
{};


} // namespace dispatch
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_IMPLEMENTATION_HPP
