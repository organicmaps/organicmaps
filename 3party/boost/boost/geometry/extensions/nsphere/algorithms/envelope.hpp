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

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_ENVELOPE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_ENVELOPE_HPP


#include <boost/geometry/algorithms/envelope.hpp>

#include <boost/geometry/extensions/nsphere/core/radius.hpp>
#include <boost/geometry/extensions/nsphere/core/tags.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace envelope
{

/// Calculate envelope of an n-sphere, circle or sphere (currently only for Cartesian 2D points)
struct envelope_nsphere
{
    template <typename Nsphere, typename Box>
    static inline void apply(Nsphere const& nsphere, Box& mbr)
    {
        assert_dimension<Nsphere, 2>();
        assert_dimension<Box, 2>();

        typename radius_type<Nsphere>::type radius = get_radius<0>(nsphere);
        set<min_corner, 0>(mbr, get<0>(nsphere) - radius);
        set<min_corner, 1>(mbr, get<1>(nsphere) - radius);
        set<max_corner, 0>(mbr, get<0>(nsphere) + radius);
        set<max_corner, 1>(mbr, get<1>(nsphere) + radius);
    }
};


}} // namespace detail::envelope
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template <typename Nsphere>
struct envelope<Nsphere, nsphere_tag>
    : detail::envelope::envelope_nsphere
{};


} // namespace dispatch
#endif



}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_ENVELOPE_HPP
