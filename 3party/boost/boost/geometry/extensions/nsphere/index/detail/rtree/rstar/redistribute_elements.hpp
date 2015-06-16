// Boost.Geometry Index
//
// R-tree R*-tree split algorithm implementation
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP

#include <boost/geometry/index/detail/rtree/rstar/redistribute_elements.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace rstar {

template <typename Element, typename Translator, size_t AxisIndex>
class element_axis_corner_less<Element, Translator, nsphere_tag, min_corner, AxisIndex>
{
public:
    element_axis_corner_less(Translator const& tr)
        : m_tr(tr)
    {}

    bool operator()(Element const& e1, Element const& e2) const
    {
        typedef typename rtree::element_indexable_type<Element, Translator>::type indexable_type;
        indexable_type const& i1 = rtree::element_indexable(e1, m_tr);
        indexable_type const& i2 = rtree::element_indexable(e2, m_tr);

        return geometry::get<AxisIndex>(i1) - get_radius<0>(i1)
             < geometry::get<AxisIndex>(i2) - get_radius<0>(i2);
    }

private:
    Translator const& m_tr;
};

template <typename Element, typename Translator, size_t AxisIndex>
class element_axis_corner_less<Element, Translator, nsphere_tag, max_corner, AxisIndex>
{
public:
    element_axis_corner_less(Translator const& tr)
        : m_tr(tr)
    {}

    bool operator()(Element const& e1, Element const& e2) const
    {
        typedef typename rtree::element_indexable_type<Element, Translator>::type indexable_type;
        indexable_type const& i1 = rtree::element_indexable(e1, m_tr);
        indexable_type const& i2 = rtree::element_indexable(e2, m_tr);

        return geometry::get<AxisIndex>(i1) + get_radius<0>(i1)
             < geometry::get<AxisIndex>(i2) + get_radius<0>(i2);
    }

private:
    Translator const& m_tr;
};

template <typename Box, size_t AxisIndex>
struct choose_split_axis_and_index_for_axis<Box, AxisIndex, nsphere_tag>
    : choose_split_axis_and_index_for_axis<Box, AxisIndex, box_tag>
{};


}}} // namespace detail::rtree::rstar

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_RTREE_RSTAR_REDISTRIBUTE_ELEMENTS_HPP
