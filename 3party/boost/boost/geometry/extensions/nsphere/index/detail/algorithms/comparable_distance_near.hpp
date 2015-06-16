// Boost.Geometry Index
//
// squared distance between point and nearest point of the box or point
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_COMPARABLE_DISTANCE_NEAR_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_COMPARABLE_DISTANCE_NEAR_HPP

#include <boost/geometry/index/detail/algorithms/comparable_distance_near.hpp>

#include <boost/geometry/extensions/nsphere/views/center_view.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

template <
    typename Point,
    typename Indexable,
    size_t N>
struct sum_for_indexable<Point, Indexable, nsphere_tag, comparable_distance_near_tag, N>
{
    typedef typename default_distance_result<Point, center_view<const Indexable> >::type result_type;

    inline static result_type apply(Point const& pt, Indexable const& i)
    {
        result_type center_dist = math::sqrt( comparable_distance(pt, center_view<const Indexable>(i)) );
        result_type dist = get_radius<0>(i) < center_dist ? center_dist - get_radius<0>(i) : 0;
        return dist;

        // return dist * dist to be conformant with comparable_distance?
        // CONSIDER returning negative value related to the distance or normalized distance to the center if dist < radius
    }
};

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_COMPARABLE_DISTANCE_NEAR_HPP
