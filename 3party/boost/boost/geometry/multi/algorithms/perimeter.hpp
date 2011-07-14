// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_MULTI_ALGORITHMS_PERIMETER_HPP
#define BOOST_GEOMETRY_MULTI_ALGORITHMS_PERIMETER_HPP


#include <boost/range/metafunctions.hpp>

#include <boost/geometry/algorithms/perimeter.hpp>

#include <boost/geometry/multi/core/tags.hpp>

#include <boost/geometry/multi/algorithms/detail/multi_sum.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{
template <typename MultiPolygon, typename Strategy>
struct perimeter<multi_polygon_tag, MultiPolygon, Strategy>
    : detail::multi_sum
        <
            typename default_length_result<MultiPolygon>::type,
            MultiPolygon,
            Strategy,
            perimeter
                <
                    polygon_tag,
                    typename boost::range_value<MultiPolygon>::type,
                    Strategy
                >
        >
{};

} // namespace dispatch
#endif


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_MULTI_ALGORITHMS_PERIMETER_HPP
