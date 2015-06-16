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

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_BOX_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_BOX_HPP

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/coordinate_system.hpp>
#include <boost/geometry/core/tags.hpp>

#include <boost/geometry/strategies/strategy_transform.hpp>

#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/algorithms/transform.hpp>
#include <boost/geometry/algorithms/detail/normalize.hpp>

#include <boost/geometry/algorithms/dispatch/envelope.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{


struct envelope_box_on_spheroid
{
    template<typename BoxIn, typename BoxOut>
    static inline void apply(BoxIn const& box_in, BoxOut& mbr)
    {
        BoxIn box_in_normalized = detail::return_normalized<BoxIn>(box_in);

        geometry::transform(box_in_normalized, mbr);
    }
};


template <typename CSTag>
struct envelope_box
{
    template<typename BoxIn, typename BoxOut>
    static inline void apply(BoxIn const& box_in, BoxOut& mbr)
    {
        geometry::convert(box_in, mbr);
    }
};

template <>
struct envelope_box<spherical_equatorial_tag>
    : envelope_box_on_spheroid
{};

template <>
struct envelope_box<geographic_tag>
    : envelope_box_on_spheroid
{};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Box>
struct envelope<Box, box_tag>
    : detail::envelope::envelope_box<typename cs_tag<Box>::type>
{};


} // namespace dispatch
#endif

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_ENVELOPE_BOX_HPP
