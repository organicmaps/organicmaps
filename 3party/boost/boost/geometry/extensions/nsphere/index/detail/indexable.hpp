// Boost.Geometry Index
//
// Indexable's traits and related functions
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/geometry/index/detail/indexable.hpp>

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_INDEXABLE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_INDEXABLE_HPP

namespace boost { namespace geometry { namespace index { namespace detail {

namespace dispatch {

template <typename Indexable>
struct point_type<Indexable, geometry::nsphere_tag>
{
    typedef typename geometry::traits::point_type<Indexable>::type type;
};

} // namespace dispatch

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_INDEXABLE_HPP
