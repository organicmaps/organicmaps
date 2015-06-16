// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_STRATEGIES_CARTESIAN_NSPHERE_IN_BOX_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_STRATEGIES_CARTESIAN_NSPHERE_IN_BOX_HPP


#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/strategies/covered_by.hpp>
#include <boost/geometry/strategies/within.hpp>


namespace boost { namespace geometry { namespace strategy
{

namespace within
{

struct nsphere_within_range
{
    template <typename ContainedValue, typename ContainingValue>
    static inline bool apply(ContainedValue const& ed_min
                           , ContainedValue const& ed_max
                           , ContainingValue const& ing_min
                           , ContainingValue const& ing_max)
    {
        return ing_min < ed_min && ed_max < ing_max;
    }
};


struct nsphere_covered_by_range
{
    template <typename ContainedValue, typename ContainingValue>
    static inline bool apply(ContainedValue const& ed_min
                           , ContainedValue const& ed_max
                           , ContainingValue const& ing_min
                           , ContainingValue const& ing_max)
    {
        return ing_min <= ed_min && ed_max <= ing_max;
    }
};

template
<
    typename SubStrategy,
    typename NSphere,
    typename Box,
    std::size_t Dimension,
    std::size_t DimensionCount
>
struct relate_nsphere_box_loop
{
    static inline bool apply(NSphere const& sphere, Box const& box)
    {
        if (! SubStrategy::apply(
                get<Dimension>(sphere) - get_radius<0>(sphere),
                get<Dimension>(sphere) + get_radius<0>(sphere),
                get<min_corner, Dimension>(box),
                get<max_corner, Dimension>(box))
            )
        {
            return false;
        }

        return relate_nsphere_box_loop
            <
                SubStrategy,
                NSphere, Box,
                Dimension + 1, DimensionCount
            >::apply(sphere, box);
    }
};


template
<
    typename SubStrategy,
    typename NSphere,
    typename Box,
    std::size_t DimensionCount
>
struct relate_nsphere_box_loop<SubStrategy, NSphere, Box, DimensionCount, DimensionCount>
{
    static inline bool apply(NSphere const& , Box const& )
    {
        return true;
    }
};


template
<
    typename NSphere,
    typename Box,
    typename SubStrategy = nsphere_within_range
>
struct nsphere_in_box
{
    static inline bool apply(NSphere const& nsphere, Box const& box)
    {
        return relate_nsphere_box_loop
            <
                SubStrategy,
                NSphere, Box,
                0, dimension<NSphere>::type::value
            >::apply(nsphere, box);
    }
};


} // namespace within


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


namespace within { namespace services
{

template <typename NSphere, typename Box>
struct default_strategy
    <
        nsphere_tag, box_tag,
        nsphere_tag, areal_tag,
        cartesian_tag, cartesian_tag,
        NSphere, Box
    >
{
    typedef within::nsphere_in_box<NSphere, Box, within::nsphere_within_range> type;
};


}} // namespace within::services


namespace covered_by { namespace services
{


template <typename NSphere, typename Box>
struct default_strategy
    <
        nsphere_tag, box_tag,
        nsphere_tag, areal_tag,
        cartesian_tag, cartesian_tag,
        NSphere, Box
    >
{
    typedef within::nsphere_in_box<NSphere, Box, within::nsphere_covered_by_range> type;
};


}} // namespace covered_by::services


#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}}} // namespace boost::geometry::strategy


#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_STRATEGIES_CARTESIAN_NSPHERE_IN_BOX_HPP
