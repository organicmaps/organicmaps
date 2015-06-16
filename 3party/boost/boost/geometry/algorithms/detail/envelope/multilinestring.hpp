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

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_MULTILINESTRING_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_MULTILINESTRING_HPP

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/expand.hpp>

#include <boost/geometry/algorithms/detail/envelope/linestring.hpp>
#include <boost/geometry/algorithms/detail/envelope/range.hpp>

#include <boost/geometry/algorithms/dispatch/envelope.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


struct envelope_range_expand_policy
{
    template <typename Box, typename Geometry>
    static inline void apply(Box& mbr, Geometry const& geometry)
    {
        Box mbr2;
        envelope_range<>::apply(geometry, mbr2);
        geometry::expand(mbr, mbr2);
    }
};


template <typename CSTag>
struct envelope_multilinestring
    : envelope_range<envelope_range_expand_policy>
{};

template <>
struct envelope_multilinestring<spherical_equatorial_tag>
    : envelope_linear_on_spheroid
{};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename MultiLinestring>
struct envelope<MultiLinestring, multi_linestring_tag>
    : detail::envelope::envelope_multilinestring
        <
            typename cs_tag<MultiLinestring>::type
        >
{};


} // namespace dispatch
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_MULTILINESTRING_HPP
