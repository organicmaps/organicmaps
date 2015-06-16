// Boost.Geometry Index
//
// n-dimensional margin value (hypersurface), 2d perimeter, 3d surface, etc...
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_MARGIN_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_MARGIN_HPP

#include <boost/geometry/index/detail/algorithms/margin.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

template <typename NSphere, size_t Dimension>
struct comparable_margin_nsphere
{
    BOOST_STATIC_ASSERT(1 < Dimension);
    //BOOST_STATIC_ASSERT(Dimension <= dimension<NSphere>::value);

    static inline typename default_margin_result<NSphere>::type apply(NSphere const& s)
    {
        return comparable_margin_nsphere<NSphere, Dimension-1>::apply(s)
            * geometry::get_radius<0>(s);
    }
};

template <typename NSphere>
struct comparable_margin_nsphere<NSphere, 1>
{
    static inline typename default_margin_result<NSphere>::type apply(NSphere const& )
    {
        return 1;
    }
};

namespace dispatch {

template <typename NSphere>
struct comparable_margin<NSphere, nsphere_tag>
{
    typedef typename default_margin_result<NSphere>::type result_type;

    static inline result_type apply(NSphere const& g)
    {
        return comparable_margin_nsphere<
            NSphere, dimension<NSphere>::value
        >::apply(g);
    }
};

} // namespace dispatch

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_MARGIN_HPP
