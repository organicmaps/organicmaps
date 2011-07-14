// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_AREA_HPP
#define BOOST_GEOMETRY_ALGORITHMS_AREA_HPP

#include <boost/concept_check.hpp>
#include <boost/mpl/if.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/metafunctions.hpp>


#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/exterior_ring.hpp>
#include <boost/geometry/core/interior_rings.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/ring_type.hpp>

#include <boost/geometry/geometries/concepts/check.hpp>

#include <boost/geometry/algorithms/detail/calculate_null.hpp>
#include <boost/geometry/algorithms/detail/calculate_sum.hpp>

#include <boost/geometry/strategies/area.hpp>
#include <boost/geometry/strategies/default_area_result.hpp>

#include <boost/geometry/strategies/concepts/area_concept.hpp>

#include <boost/geometry/util/math.hpp>
#include <boost/geometry/util/order_as_direction.hpp>
#include <boost/geometry/views/closeable_view.hpp>
#include <boost/geometry/views/reversible_view.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace area
{

template<typename Box, typename Strategy>
struct box_area
{
    typedef typename coordinate_type<Box>::type return_type;

    static inline return_type apply(Box const& box, Strategy const&)
    {
        // Currently only works for 2D Cartesian boxes
        assert_dimension<Box, 2>();

        return_type const dx = get<max_corner, 0>(box)
                - get<min_corner, 0>(box);
        return_type const dy = get<max_corner, 1>(box)
                - get<min_corner, 1>(box);

        return dx * dy;
    }
};


template
<
    typename Ring,
    iterate_direction Direction,
    closure_selector Closure,
    typename Strategy
>
struct ring_area
{
    BOOST_CONCEPT_ASSERT( (geometry::concept::AreaStrategy<Strategy>) );

    typedef typename Strategy::return_type type;

    static inline type apply(Ring const& ring, Strategy const& strategy)
    {
        assert_dimension<Ring, 2>();

        // Ignore warning (because using static method sometimes) on strategy
        boost::ignore_unused_variable_warning(strategy);

        // An open ring has at least three points,
        // A closed ring has at least four points,
        // if not, there is no (zero) area
        if (boost::size(ring)
                < core_detail::closure::minimum_ring_size<Closure>::value)
        {
            return type();
        }

        typedef typename reversible_view<Ring const, Direction>::type rview_type;
        typedef typename closeable_view
            <
                rview_type const, Closure
            >::type view_type;
        typedef typename boost::range_iterator<view_type const>::type iterator_type;

        rview_type rview(ring);
        view_type view(rview);
        typename Strategy::state_type state;
        iterator_type it = boost::begin(view);
        iterator_type end = boost::end(view);

        for (iterator_type previous = it++;
            it != end;
            ++previous, ++it)
        {
            strategy.apply(*previous, *it, state);
        }

        return strategy.result(state);
    }
};


}} // namespace detail::area


#endif // DOXYGEN_NO_DETAIL


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template
<
    typename Tag,
    typename Geometry,
    typename Strategy
>
struct area
    : detail::calculate_null
        <
            typename Strategy::return_type,
            Geometry,
            Strategy
        > {};


template
<
    typename Geometry,
    typename Strategy
>
struct area<box_tag, Geometry, Strategy>
    : detail::area::box_area<Geometry, Strategy>
{};


template
<
    typename Ring,
    typename Strategy
>
struct area<ring_tag, Ring, Strategy>
    : detail::area::ring_area
        <
            Ring,
            order_as_direction<geometry::point_order<Ring>::value>::value,
            geometry::closure<Ring>::value,
            Strategy
        >
{};


template
<
    typename Polygon,
    typename Strategy
>
struct area<polygon_tag, Polygon, Strategy>
    : detail::calculate_polygon_sum
        <
            typename Strategy::return_type,
            Polygon,
            Strategy,
            detail::area::ring_area
                <
                    typename ring_type<Polygon const>::type,
                    order_as_direction<geometry::point_order<Polygon>::value>::value,
                    geometry::closure<Polygon>::value,
                    Strategy
                >
        >
{};


} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH



/*!
\brief \brief_calc{area}
\ingroup area
\details \details_calc{area}. \details_default_strategy

The area algorithm calculates the surface area of all geometries having a surface, namely
box, polygon, ring, multipolygon. The units are the square of the units used for the points
defining the surface. If subject geometry is defined in meters, then area is calculated
in square meters.

The area calculation can be done in all three common coordinate systems, Cartesian, Spherical
and Geographic as well.

\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{area}

\qbk{[include reference/algorithms/area.qbk]}
\qbk{[heading Examples]}
\qbk{[area] [area_output]}
*/
template <typename Geometry>
inline typename default_area_result<Geometry>::type area(Geometry const& geometry)
{
    concept::check<Geometry const>();

    typedef typename point_type<Geometry>::type point_type;
    typedef typename strategy::area::services::default_strategy
        <
            typename cs_tag<point_type>::type,
            point_type
        >::type strategy_type;

    return dispatch::area
        <
            typename tag<Geometry>::type,
            Geometry,
            strategy_type
        >::apply(geometry, strategy_type());
}

/*!
\brief \brief_calc{area} \brief_strategy
\ingroup area
\details \details_calc{area} \brief_strategy. \details_strategy_reasons
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{Area}
\param geometry \param_geometry
\param strategy \param_strategy{area}
\return \return_calc{area}

\qbk{distinguish,with strategy}

\qbk{
[include reference/algorithms/area.qbk]

[heading Example]
[area_with_strategy]
[area_with_strategy_output]

[heading Available Strategies]
\* [link geometry.reference.strategies.strategy_area_surveyor Surveyor (cartesian)]
\* [link geometry.reference.strategies.strategy_area_huiller Huiller (spherical)]
}
 */
template <typename Geometry, typename Strategy>
inline typename Strategy::return_type area(
        Geometry const& geometry, Strategy const& strategy)
{
    concept::check<Geometry const>();

    return dispatch::area
        <
            typename tag<Geometry>::type,
            Geometry,
            Strategy
        >::apply(geometry, strategy);
}


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_ALGORITHMS_AREA_HPP
