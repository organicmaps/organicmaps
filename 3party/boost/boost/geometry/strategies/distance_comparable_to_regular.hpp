// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_STRATEGIES_DISTANCE_COMPARABLE_TO_REGULAR_HPP
#define BOOST_GEOMETRY_STRATEGIES_DISTANCE_COMPARABLE_TO_REGULAR_HPP

#include <boost/geometry/core/tags.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/algorithms/not_implemented.hpp>


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
    typename Geometry2,
    typename CsTag1 = typename cs_tag<Geometry1>::type,
    typename CsTag2 = typename cs_tag<Geometry2>::type
>
struct comparable_to_regular
    : geometry::not_implemented<CsTag1, CsTag2>
{};


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_STRATEGIES_DISTANCE_COMPARABLE_TO_REGULAR_HPP
