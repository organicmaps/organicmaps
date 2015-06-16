// Boost.Geometry Index
//
// n-dimensional content (hypervolume) - 2d area, 3d volume, ...
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_CONTENT_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_CONTENT_HPP

#include <boost/geometry/index/detail/algorithms/content.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

namespace dispatch {

// TODO replace it by comparable_content?
// probably radius^Dimension would be sufficient
// WARNING! this would work only if the same Geometries were compared
// so it shouldn't be used in the case of Variants!
// The same with margin()!

template <typename NSphere, size_t Dimension>
struct content_nsphere
{
    BOOST_STATIC_ASSERT(2 < Dimension);

    typedef typename detail::default_content_result<NSphere>::type result_type;

    static inline result_type apply(NSphere const& s)
    {
        return (content_nsphere<NSphere, Dimension - 2>::apply(s)
                    * 2 * get_radius<0>(s) * get_radius<0>(s)
                    * ::boost::math::constants::pi<result_type>()) / Dimension;
    }
};

template <typename NSphere>
struct content_nsphere<NSphere, 2>
{
    typedef typename detail::default_content_result<NSphere>::type result_type;

    static inline result_type apply(NSphere const& s)
    {
        return ::boost::math::constants::pi<result_type>() * get_radius<0>(s) * get_radius<0>(s);
    }
};

template <typename NSphere>
struct content_nsphere<NSphere, 1>
{
    typedef typename detail::default_content_result<NSphere>::type result_type;

    static inline result_type apply(NSphere const& s)
    {
        return 2 * get_radius<0>(s);
    }
};

template <typename Indexable>
struct content<Indexable, nsphere_tag>
{
    static typename default_content_result<Indexable>::type apply(Indexable const& i)
    {
        return dispatch::content_nsphere<Indexable, dimension<Indexable>::value>::apply(i);
    }
};

} // namespace dispatch

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_ALGORITHMS_CONTENT_HPP
