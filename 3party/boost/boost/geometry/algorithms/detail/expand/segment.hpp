// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2015 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2015 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2015 Mateusz Loskot, London, UK.
// Copyright (c) 2014-2015 Samuel Debionne, Grenoble, France.

// This file was modified by Oracle on 2015.
// Modifications copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_EXPAND_SEGMENT_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_EXPAND_SEGMENT_HPP

#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/algorithms/detail/envelope/segment.hpp>
#include <boost/geometry/algorithms/detail/expand/box.hpp>
#include <boost/geometry/algorithms/detail/expand/indexed.hpp>
#include <boost/geometry/algorithms/dispatch/expand.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace expand
{


struct segment_on_spheroid
{
    template <typename Box, typename Segment>
    static inline void apply(Box& box, Segment const& segment)
    {
        Box segment_envelope;
        dispatch::envelope<Segment>::apply(segment, segment_envelope);
        box_on_spheroid::apply(box, segment_envelope);
    }
};


}} // namespace detail::expand
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Box, typename Segment,
    typename StrategyLess, typename StrategyGreater,
    typename CSTagOut, typename CSTag
>
struct expand
    <
        Box, Segment, StrategyLess, StrategyGreater,
        box_tag, segment_tag, CSTagOut, CSTag
    > : detail::expand::expand_indexed<StrategyLess, StrategyGreater>
{};

template
<
    typename Box, typename Segment,
    typename StrategyLess, typename StrategyGreater
>
struct expand
    <
        Box, Segment, StrategyLess, StrategyGreater,
        box_tag, segment_tag,
        spherical_equatorial_tag, spherical_equatorial_tag
    > : detail::expand::segment_on_spheroid
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_EXPAND_SEGMENT_HPP
