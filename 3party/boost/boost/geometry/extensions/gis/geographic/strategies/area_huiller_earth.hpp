// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_GIS_GEOGRAPHIC_STRATEGIES_AREA_HUILLER_EARTH_HPP
#define BOOST_GEOMETRY_EXTENSIONS_GIS_GEOGRAPHIC_STRATEGIES_AREA_HUILLER_EARTH_HPP



#include <boost/geometry/strategies/spherical/area_huiller.hpp>


namespace boost { namespace geometry
{

namespace strategy { namespace area
{

template
<
    typename PointOfSegment,
    typename CalculationType = void
>
class huiller_earth
    : public huiller<PointOfSegment, CalculationType>
{
public :
    // By default the average earth radius.
    // Uses can specify another radius.
    // Note that the earth is still handled spherically
    inline huiller_earth(double radius = 6372795.0)
        : huiller<PointOfSegment, CalculationType>(radius)
    {}
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS

namespace services
{

template <typename Point>
struct default_strategy<geographic_tag, Point>
{
    typedef huiller_earth<Point> type;
};

} // namespace services


#endif


}} // namespace strategy::area


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_GIS_GEOGRAPHIC_STRATEGIES_AREA_HUILLER_EARTH_HPP
