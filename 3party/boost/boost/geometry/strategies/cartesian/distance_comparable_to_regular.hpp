// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_COMPARABLE_TO_REGULAR_HPP
#define BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_COMPARABLE_TO_REGULAR_HPP


#include <cmath>
#include <boost/geometry/strategies/distance_comparable_to_regular.hpp>
#include <boost/geometry/core/point_type.hpp>
#include <boost/geometry/util/math.hpp>

namespace boost { namespace geometry
{

namespace strategy { namespace distance
{



#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{


template
<
    typename ComparableStrategy,
    typename Strategy,
    typename Geometry1,
    typename Geometry2
>
struct comparable_to_regular
    <
        ComparableStrategy, Strategy,
        Geometry1, Geometry2,
        cartesian_tag, cartesian_tag
    >
{
    typedef typename return_type
        <
            Strategy,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >::type calculation_type;

    typedef typename return_type
        <
            ComparableStrategy,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >::type comparable_calculation_type;

    static inline calculation_type apply(comparable_calculation_type const& cd)
    {
        return math::sqrt( boost::numeric_cast<calculation_type>(cd) );
    }
};



template <typename ComparableStrategy, typename Geometry1, typename Geometry2>
struct comparable_to_regular
    <
        ComparableStrategy,
        ComparableStrategy,
        Geometry1,
        Geometry2,
        cartesian_tag,
        cartesian_tag
    >
{
    typedef typename return_type
        <
            ComparableStrategy,
            typename point_type<Geometry1>::type,
            typename point_type<Geometry2>::type
        >::type comparable_calculation_type;

    static inline comparable_calculation_type
    apply(comparable_calculation_type const& cd)
    {
        return cd;
    }
};







} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_CARTESIAN_DISTANCE_COMPARABLE_TO_REGULAR_HPP
