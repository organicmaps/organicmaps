// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_EXPAND_HPP
#define BOOST_GEOMETRY_ALGORITHMS_EXPAND_HPP


#include <cstddef>

#include <boost/numeric/conversion/cast.hpp>

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
    typename Box, typename Point,
    typename StrategyLess, typename StrategyGreater,
    std::size_t Dimension, std::size_t DimensionCount
>
struct point_loop
{
    typedef typename strategy::compare::detail::select_strategy
        <
            StrategyLess, 1, Point, Dimension
        >::type less_type;

    typedef typename strategy::compare::detail::select_strategy
        <
            StrategyGreater, -1, Point, Dimension
        >::type greater_type;

    typedef typename select_coordinate_type<Point, Box>::type coordinate_type;

    static inline void apply(Box& box, Point const& source)
    {
        less_type less;
        greater_type greater;

        coordinate_type const coord = get<Dimension>(source);

        if (less(coord, get<min_corner, Dimension>(box)))
        {
            set<min_corner, Dimension>(box, coord);
        }

        if (greater(coord, get<max_corner, Dimension>(box)))
        {
            set<max_corner, Dimension>(box, coord);
        }

        point_loop
            <
                Box, Point,
                StrategyLess, StrategyGreater,
                Dimension + 1, DimensionCount
            >::apply(box, source);
    }
};


template
<
    typename Box, typename Point,
    typename StrategyLess, typename StrategyGreater,
    std::size_t DimensionCount
>
struct point_loop
    <
        Box, Point,
        StrategyLess, StrategyGreater,
        DimensionCount, DimensionCount
    >
{
    static inline void apply(Box&, Point const&) {}
};


template
<
    typename Box, typename Geometry,
    typename StrategyLess, typename StrategyGreater,
    std::size_t Index,
    std::size_t Dimension, std::size_t DimensionCount
>
struct indexed_loop
{
    typedef typename strategy::compare::detail::select_strategy
        <
            StrategyLess, 1, Box, Dimension
        >::type less_type;

    typedef typename strategy::compare::detail::select_strategy
        <
            StrategyGreater, -1, Box, Dimension
        >::type greater_type;

    typedef typename select_coordinate_type
            <
                Box,
                Geometry
            >::type coordinate_type;


    static inline void apply(Box& box, Geometry const& source)
    {
        less_type less;
        greater_type greater;

        coordinate_type const coord = get<Index, Dimension>(source);

        if (less(coord, get<min_corner, Dimension>(box)))
        {
            set<min_corner, Dimension>(box, coord);
        }

        if (greater(coord, get<max_corner, Dimension>(box)))
        {
            set<max_corner, Dimension>(box, coord);
        }

        indexed_loop
            <
                Box, Geometry,
                StrategyLess, StrategyGreater,
                Index, Dimension + 1, DimensionCount
            >::apply(box, source);
    }
};


template
<
    typename Box, typename Geometry,
    typename StrategyLess, typename StrategyGreater,
    std::size_t Index, std::size_t DimensionCount
>
struct indexed_loop
    <
        Box, Geometry,
        StrategyLess, StrategyGreater,
        Index, DimensionCount, DimensionCount
    >
{
    static inline void apply(Box&, Geometry const&) {}
};



// Changes a box such that the other box is also contained by the box
template
<
    typename Box, typename Geometry,
    typename StrategyLess, typename StrategyGreater
>
struct expand_indexed
{
    static inline void apply(Box& box, Geometry const& geometry)
    {
        indexed_loop
            <
                Box, Geometry,
                StrategyLess, StrategyGreater,
                0, 0, dimension<Geometry>::type::value
            >::apply(box, geometry);

        indexed_loop
            <
                Box, Geometry,
                StrategyLess, StrategyGreater,
                1, 0, dimension<Geometry>::type::value
            >::apply(box, geometry);
    }
};

}} // namespace detail::expand
#endif // DOXYGEN_NO_DETAIL

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename BoxOut, typename Geometry,
    typename StrategyLess, typename StrategyGreater
>
struct expand
{};


// Box + point -> new box containing also point
template
<
    typename BoxOut, typename Point,
    typename StrategyLess, typename StrategyGreater
>
struct expand<point_tag, BoxOut, Point, StrategyLess, StrategyGreater>
    : detail::expand::point_loop
        <
            BoxOut, Point,
            StrategyLess, StrategyGreater,
            0, dimension<Point>::type::value
        >
{};


// Box + box -> new box containing two input boxes
template
<
    typename BoxOut, typename BoxIn,
    typename StrategyLess, typename StrategyGreater
>
struct expand<box_tag, BoxOut, BoxIn, StrategyLess, StrategyGreater>
    : detail::expand::expand_indexed
        <BoxOut, BoxIn, StrategyLess, StrategyGreater>
{};

template
<
    typename Box, typename Segment,
    typename StrategyLess, typename StrategyGreater
>
struct expand<segment_tag, Box, Segment, StrategyLess, StrategyGreater>
    : detail::expand::expand_indexed
        <Box, Segment, StrategyLess, StrategyGreater>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/***
*!
\brief Expands a box using the extend (envelope) of another geometry (box, point)
\ingroup expand
\tparam Box type of the box
\tparam Geometry of second geometry, to be expanded with the box
\param box box to expand another geometry with, might be changed
\param geometry other geometry
\param strategy_less
\param strategy_greater
\note Strategy is currently ignored
 *
template
<
    typename Box, typename Geometry,
    typename StrategyLess, typename StrategyGreater
>
inline void expand(Box& box, Geometry const& geometry,
            StrategyLess const& strategy_less,
            StrategyGreater const& strategy_greater)
{
    concept::check_concepts_and_equal_dimensions<Box, Geometry const>();

    dispatch::expand
        <
            typename tag<Geometry>::type,
            Box,
            Geometry,
            StrategyLess, StrategyGreater
        >::apply(box, geometry);
}
***/


/*!
\brief Expands a box using the bounding box (envelope) of another geometry (box, point)
\ingroup expand
\tparam Box type of the box
\tparam Geometry \tparam_geometry
\param box box to be expanded using another geometry, mutable
\param geometry \param_geometry geometry which envelope (bounding box) will be added to the box

\qbk{[include reference/algorithms/expand.qbk]}
 */
template <typename Box, typename Geometry>
inline void expand(Box& box, Geometry const& geometry)
{
    concept::check_concepts_and_equal_dimensions<Box, Geometry const>();

    dispatch::expand
        <
            typename tag<Geometry>::type,
            Box, Geometry,
            strategy::compare::default_strategy,
            strategy::compare::default_strategy
        >::apply(box, geometry);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_EXPAND_HPP
