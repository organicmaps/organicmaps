// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_EXPAND_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_EXPAND_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/algorithms/not_implemented.hpp>
#include <boost/geometry/core/coordinate_dimension.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/util/select_coordinate_type.hpp>

#include <boost/geometry/strategies/compare.hpp>
#include <boost/geometry/policies/compare.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace expand
{


template
<
    typename StrategyLess, typename StrategyGreater,
    std::size_t Dimension, std::size_t DimensionCount
>
struct nsphere_loop
{
    template <typename Box, typename NSphere>
    static inline void apply(Box& box, NSphere const& source)
    {
        typedef typename strategy::compare::detail::select_strategy
            <
                StrategyLess, 1, NSphere, Dimension
            >::type less_type;

        typedef typename strategy::compare::detail::select_strategy
            <
                StrategyGreater, -1, NSphere, Dimension
            >::type greater_type;

        typedef typename select_coordinate_type<NSphere, Box>::type coordinate_type;

        less_type less;
        greater_type greater;

        coordinate_type const min_coord = get<Dimension>(source) - get_radius<0>(source);
        coordinate_type const max_coord = get<Dimension>(source) + get_radius<0>(source);

        if (less(min_coord, get<min_corner, Dimension>(box)))
        {
            set<min_corner, Dimension>(box, min_coord);
        }

        if (greater(max_coord, get<max_corner, Dimension>(box)))
        {
            set<max_corner, Dimension>(box, max_coord);
        }

        nsphere_loop
            <
                StrategyLess, StrategyGreater,
                Dimension + 1, DimensionCount
            >::apply(box, source);
    }
};


template
<
    typename StrategyLess, typename StrategyGreater,
    std::size_t DimensionCount
>
struct nsphere_loop
    <
        StrategyLess, StrategyGreater,
        DimensionCount, DimensionCount
    >
{
    template <typename Box, typename NSphere>
    static inline void apply(Box&, NSphere const&) {}
};


}} // namespace detail::expand
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


// Box + Nsphere -> new box containing also nsphere
template
<
    typename BoxOut, typename NSphere,
    typename StrategyLess, typename StrategyGreater
>
struct expand<BoxOut, NSphere, StrategyLess, StrategyGreater, box_tag, nsphere_tag>
    : detail::expand::nsphere_loop
        <
            StrategyLess, StrategyGreater,
            0, dimension<NSphere>::type::value
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_ALGORITHMS_EXPAND_HPP
