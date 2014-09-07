// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2014 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2014 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2014 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_APPEND_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_APPEND_HPP

#include <boost/geometry/algorithms/append.hpp>

#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace append
{


template <typename MultiGeometry, typename RangeOrPoint>
struct append_to_multigeometry
{
    static inline void apply(MultiGeometry& multigeometry,
                             RangeOrPoint const& range_or_point,
                             int ring_index, int multi_index)
    {
        
        dispatch::append
            <
                typename boost::range_value<MultiGeometry>::type,
                RangeOrPoint
            >::apply(multigeometry[multi_index], range_or_point, ring_index);
    }
};


}} // namespace detail::append
#endif // DOXYGEN_NO_DETAIL



#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

namespace splitted_dispatch
{

template <typename Geometry, typename Point>
struct append_point<multi_point_tag, Geometry, Point>
    : detail::append::append_point<Geometry, Point>
{};

template <typename Geometry, typename Range>
struct append_range<multi_point_tag, Geometry, Range>
    : detail::append::append_range<Geometry, Range>
{};

template <typename MultiGeometry, typename RangeOrPoint>
struct append_point<multi_linestring_tag, MultiGeometry, RangeOrPoint>
    : detail::append::append_to_multigeometry<MultiGeometry, RangeOrPoint>
{};

template <typename MultiGeometry, typename RangeOrPoint>
struct append_range<multi_linestring_tag, MultiGeometry, RangeOrPoint>
    : detail::append::append_to_multigeometry<MultiGeometry, RangeOrPoint>
{};

template <typename MultiGeometry, typename RangeOrPoint>
struct append_point<multi_polygon_tag, MultiGeometry, RangeOrPoint>
    : detail::append::append_to_multigeometry<MultiGeometry, RangeOrPoint>
{};

template <typename MultiGeometry, typename RangeOrPoint>
struct append_range<multi_polygon_tag, MultiGeometry, RangeOrPoint>
    : detail::append::append_to_multigeometry<MultiGeometry, RangeOrPoint>
{};

}


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_APPEND_HPP
