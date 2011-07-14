// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2011 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2011 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2011 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_PERIMETER_HPP
#define BOOST_GEOMETRY_ALGORITHMS_PERIMETER_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/geometries/concepts/check.hpp>
#include <boost/geometry/strategies/default_length_result.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/algorithms/detail/calculate_null.hpp>
#include <boost/geometry/algorithms/detail/calculate_sum.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// Default perimeter is 0.0, specializations implement calculated values
template <typename Tag, typename Geometry, typename Strategy>
struct perimeter : detail::calculate_null
    <
        typename default_length_result<Geometry>::type,
        Geometry,
        Strategy
    >
{};

template <typename Geometry, typename Strategy>
struct perimeter<ring_tag, Geometry, Strategy>
    : detail::length::range_length
        <
            Geometry,
            Strategy,
            closure<Geometry>::value
        >
{};

template <typename Polygon, typename Strategy>
struct perimeter<polygon_tag, Polygon, Strategy>
    : detail::calculate_polygon_sum
        <
            typename default_length_result<Polygon>::type,
            Polygon,
            Strategy,
            detail::length::range_length
                <
                    typename ring_type<Polygon>::type,
                    Strategy,
                    closure<Polygon>::value
                >
        >
{};


// box,n-sphere: to be implemented

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH


/*!
\brief \brief_calc{perimeter}
\ingroup perimeter
\details The function perimeter returns the perimeter of a geometry,
    using the default distance-calculation-strategy
\tparam Geometry \tparam_geometry
\param geometry \param_geometry
\return \return_calc{perimeter}

\qbk{[include reference/algorithms/perimeter.qbk]}
 */
template<typename Geometry>
inline typename default_length_result<Geometry>::type perimeter(
        Geometry const& geometry)
{
    concept::check<Geometry const>();

    typedef typename point_type<Geometry>::type point_type;
    typedef typename strategy::distance::services::default_strategy
        <
            point_tag, point_type
        >::type strategy_type;

    return dispatch::perimeter
        <
            typename tag<Geometry>::type,
            Geometry,
            strategy_type
        >::apply(geometry, strategy_type());
}

/*!
\brief \brief_calc{perimeter} \brief_strategy
\ingroup perimeter
\details The function perimeter returns the perimeter of a geometry,
    using specified strategy
\tparam Geometry \tparam_geometry
\tparam Strategy \tparam_strategy{distance}
\param geometry \param_geometry
\param strategy strategy to be used for distance calculations.
\return \return_calc{perimeter}

\qbk{distinguish,with strategy}
\qbk{[include reference/algorithms/perimeter.qbk]}
 */
template<typename Geometry, typename Strategy>
inline typename default_length_result<Geometry>::type perimeter(
        Geometry const& geometry, Strategy const& strategy)
{
    concept::check<Geometry const>();

    return dispatch::perimeter
        <
            typename tag<Geometry>::type,
            Geometry,
            Strategy
        >::apply(geometry, strategy);
}

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_PERIMETER_HPP

