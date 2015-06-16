// Boost.Geometry Index
//
// n-dimensional Indexable validity check
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_INDEX_DETAIL_ALGORITHMS_IS_VALID_HPP
#define BOOST_GEOMETRY_EXTENSIONS_INDEX_DETAIL_ALGORITHMS_IS_VALID_HPP

#include <boost/geometry/index/detail/algorithms/is_valid.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

namespace dispatch {

template <typename Indexable>
struct is_valid<Indexable, nsphere_tag>
{
    static inline bool apply(Indexable const& i)
    {
        return 0 <= geometry::get_radius<0>(i);
    }
};

} // namespace dispatch

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_EXTENSIONS_INDEX_DETAIL_ALGORITHMS_IS_VALID_HPP
