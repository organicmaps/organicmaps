// Boost.Geometry Index
//
// n-dimensional bounds
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP

#include <boost/geometry/index/detail/algorithms/bounds.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

template <typename Geometry, typename Bounds, std::size_t DimensionIndex, std::size_t DimensionCount>
struct bounds_nsphere_box
{
    static inline void apply(Geometry const& g, Bounds & b)
    {
        set<min_corner, DimensionIndex>(b, get<DimensionIndex>(g) - get_radius<0>(g));
        set<max_corner, DimensionIndex>(b, get<DimensionIndex>(g) + get_radius<0>(g));
        bounds_nsphere_box<Geometry, Bounds, DimensionIndex+1, DimensionCount>::apply(g, b);
    }
};

template <typename Geometry, typename Bounds, std::size_t DimensionCount>
struct bounds_nsphere_box<Geometry, Bounds, DimensionCount, DimensionCount>
{
    static inline void apply(Geometry const& , Bounds & ) {}
};

namespace dispatch {

template <typename Geometry, typename Bounds>
struct bounds<Geometry, Bounds, nsphere_tag, box_tag>
    : bounds_nsphere_box<Geometry, Bounds, 0, dimension<Geometry>::value>
{};

} // namespace dispatch

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_BOUNDS_HPP
