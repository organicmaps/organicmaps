// Boost.Geometry Index
//
// R-tree linear split algorithm implementation
//
// Copyright (c) 2008 Federico J. Fernandez.
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_RTREE_LINEAR_REDISTRIBUTE_ELEMENTS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_RTREE_LINEAR_REDISTRIBUTE_ELEMENTS_HPP

#include <boost/geometry/index/detail/rtree/linear/redistribute_elements.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail { namespace rtree { namespace linear {

template <typename Elements, typename Parameters, typename Translator, size_t DimensionIndex>
struct find_greatest_normalized_separation<Elements, Parameters, Translator, nsphere_tag, DimensionIndex>
{
    typedef typename Elements::value_type element_type;
    typedef typename rtree::element_indexable_type<element_type, Translator>::type indexable_type;
    typedef typename coordinate_type<indexable_type>::type coordinate_type;

    typedef typename boost::mpl::if_c<
        boost::is_integral<coordinate_type>::value,
        double,
        coordinate_type
    >::type separation_type;

    static inline void apply(Elements const& elements,
                             Parameters const& parameters,
                             Translator const& translator,
                             separation_type & separation,
                             size_t & seed1,
                             size_t & seed2)
    {
        const size_t elements_count = parameters.get_max_elements() + 1;
        BOOST_GEOMETRY_INDEX_ASSERT(elements.size() == elements_count, "unexpected number of elements");
        BOOST_GEOMETRY_INDEX_ASSERT(2 <= elements_count, "unexpected number of elements");

        indexable_type const& indexable = rtree::element_indexable(elements[0], translator);

        // find the lowest low, highest high
        coordinate_type lowest_low = geometry::get<DimensionIndex>(indexable) - geometry::get_radius<0>(indexable);
        coordinate_type highest_high = geometry::get<DimensionIndex>(indexable) + geometry::get_radius<0>(indexable);
        // and the lowest high
        coordinate_type lowest_high = highest_high;
        size_t lowest_high_index = 0;
        for ( size_t i = 1 ; i < elements_count ; ++i )
        {
            indexable_type const& indexable = rtree::element_indexable(elements[i], translator);

            coordinate_type min_coord = geometry::get<DimensionIndex>(indexable) - geometry::get_radius<0>(indexable);
            coordinate_type max_coord = geometry::get<DimensionIndex>(indexable) + geometry::get_radius<0>(indexable);

            if ( max_coord < lowest_high )
            {
                lowest_high = max_coord;
                lowest_high_index = i;
            }

            if ( min_coord < lowest_low )
                lowest_low = min_coord;

            if ( highest_high < max_coord )
                highest_high = max_coord;
        }

        // find the highest low
        size_t highest_low_index = lowest_high_index == 0 ? 1 : 0;
        indexable_type const& highest_low_indexable = rtree::element_indexable(elements[highest_low_index], translator);
        coordinate_type highest_low = geometry::get<DimensionIndex>(highest_low_indexable) - geometry::get_radius<0>(highest_low_indexable);
        for ( size_t i = highest_low_index ; i < elements_count ; ++i )
        {
            indexable_type const& indexable = rtree::element_indexable(elements[i], translator);

            coordinate_type min_coord = geometry::get<DimensionIndex>(indexable) - geometry::get_radius<0>(indexable);
            if ( highest_low < min_coord &&
                 i != lowest_high_index )
            {
                highest_low = min_coord;
                highest_low_index = i;
            }
        }

        coordinate_type const width = highest_high - lowest_low;

        // highest_low - lowest_high
        separation = difference<separation_type>(lowest_high, highest_low);
        // BOOST_GEOMETRY_INDEX_ASSERT(0 <= width);
        if ( std::numeric_limits<coordinate_type>::epsilon() < width )
            separation /= width;

        seed1 = highest_low_index;
        seed2 = lowest_high_index;

        ::boost::ignore_unused_variable_warning(parameters);
    }
};

}}} // namespace detail::rtree::linear

}}} // namespace boost::geometry::index

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_INDEX_DETAIL_RTREE_LINEAR_REDISTRIBUTE_ELEMENTS_HPP
