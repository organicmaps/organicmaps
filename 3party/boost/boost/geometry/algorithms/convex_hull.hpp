// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_HPP
#define BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_HPP



#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/exterior_ring.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/strategies/convex_hull.hpp>
#include <boost/geometry/strategies/concepts/convex_hull_concept.hpp>

#include <boost/geometry/views/detail/range_type.hpp>

#include <boost/geometry/algorithms/detail/as_range.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convex_hull
{

template
<
    typename Geometry,
    order_selector Order,
    typename Strategy
>
struct hull_insert
{

    // Member template function (to avoid inconvenient declaration
    // of output-iterator-type, from hull_to_geometry)
    template <typename OutputIterator>
    static inline OutputIterator apply(Geometry const& geometry,
            OutputIterator out, Strategy const& strategy)
    {
        typename Strategy::state_type state;

        strategy.apply(geometry, state);
        strategy.result(state, out, Order == clockwise);
        return out;
    }
};

template
<
    typename Geometry,
    typename OutputGeometry,
    typename Strategy
>
struct hull_to_geometry
{
    static inline void apply(Geometry const& geometry, OutputGeometry& out,
            Strategy const& strategy)
    {
        hull_insert
            <
                Geometry,
                geometry::point_order<OutputGeometry>::value,
                Strategy
            >::apply(geometry,
                std::back_inserter(
                    // Handle linestring, ring and polygon the same:
                    detail::as_range
                        <
                            typename range_type<OutputGeometry>::type
                        >(out)), strategy);
    }
};


}} // namespace detail::convex_hull
#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{


template
<
    typename Tag1,
    typename Geometry,
    typename Output,
    typename Strategy
>
struct convex_hull
    : detail::convex_hull::hull_to_geometry<Geometry, Output, Strategy>
{};


template
<
    typename GeometryTag,
    order_selector Order,
    typename GeometryIn, typename Strategy
 >
struct convex_hull_insert
    : detail::convex_hull::hull_insert<GeometryIn, Order, Strategy>
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


template<typename Geometry1, typename Geometry2, typename Strategy>
inline void convex_hull(Geometry1 const& geometry,
            Geometry2& out, Strategy const& strategy)
{
    concept::check_concepts_and_equal_dimensions
        <
            const Geometry1,
            Geometry2
        >();

    BOOST_CONCEPT_ASSERT( (geometry::concept::ConvexHullStrategy<Strategy>) );


    dispatch::convex_hull
        <
            typename tag<Geometry1>::type,
            Geometry1,
            Geometry2,
            Strategy
        >::apply(geometry, out, strategy);
}


/*!
\brief \brief_calc{convex hull}
\ingroup convex_hull
\details \details_calc{convex_hull,convex hull}.
\tparam Geometry1 \tparam_geometry
\tparam Geometry2 \tparam_geometry
\param geometry \param_geometry,  input geometry
\param hull \param_geometry \param_set{convex hull}

\qbk{[include reference/algorithms/convex_hull.qbk]}
 */
template<typename Geometry1, typename Geometry2>
inline void convex_hull(Geometry1 const& geometry,
            Geometry2& hull)
{
    concept::check_concepts_and_equal_dimensions
        <
            const Geometry1,
            Geometry2
        >();

    typedef typename point_type<Geometry2>::type point_type;

    typedef typename strategy_convex_hull
        <
            typename cs_tag<point_type>::type,
            Geometry1,
            point_type
        >::type strategy_type;

    convex_hull(geometry, hull, strategy_type());
}

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace convex_hull
{


template<typename Geometry, typename OutputIterator, typename Strategy>
inline OutputIterator convex_hull_insert(Geometry const& geometry,
            OutputIterator out, Strategy const& strategy)
{
    // Concept: output point type = point type of input geometry
    concept::check<Geometry const>();
    concept::check<typename point_type<Geometry>::type>();

    BOOST_CONCEPT_ASSERT( (geometry::concept::ConvexHullStrategy<Strategy>) );

    return dispatch::convex_hull_insert
        <
            typename tag<Geometry>::type,
            geometry::point_order<Geometry>::value,
            Geometry, Strategy
        >::apply(geometry, out, strategy);
}


/*!
\brief Calculate the convex hull of a geometry, output-iterator version
\ingroup convex_hull
\tparam Geometry the input geometry type
\tparam OutputIterator: an output-iterator
\param geometry the geometry to calculate convex hull from
\param out an output iterator outputing points of the convex hull
\note This overloaded version outputs to an output iterator.
In this case, nothing is known about its point-type or
    about its clockwise order. Therefore, the input point-type
    and order are copied

 */
template<typename Geometry, typename OutputIterator>
inline OutputIterator convex_hull_insert(Geometry const& geometry,
            OutputIterator out)
{
    // Concept: output point type = point type of input geometry
    concept::check<Geometry const>();
    concept::check<typename point_type<Geometry>::type>();

    typedef typename point_type<Geometry>::type point_type;

    typedef typename strategy_convex_hull
        <
            typename cs_tag<point_type>::type,
            Geometry,
            point_type
        >::type strategy_type;

    return convex_hull_insert(geometry, out, strategy_type());
}


}} // namespace detail::convex_hull
#endif // DOXYGEN_NO_DETAIL


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_CONVEX_HULL_HPP
