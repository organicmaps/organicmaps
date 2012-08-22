// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_HAVERSINE_HPP
#define BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_HAVERSINE_HPP


#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/core/access.hpp>
#include <boost/geometry/core/radian_access.hpp>

#include <boost/geometry/util/select_calculation_type.hpp>
#include <boost/geometry/util/promote_floating_point.hpp>

#include <boost/geometry/strategies/distance.hpp>



namespace boost { namespace geometry
{


namespace strategy { namespace distance
{


namespace comparable
{

// Comparable haversine.
// To compare distances, we can avoid:
// - multiplication with radius and 2.0
// - applying sqrt
// - applying asin (which is strictly (monotone) increasing)
template
<
    typename Point1,
    typename Point2 = Point1,
    typename CalculationType = void
>
class haversine
{
public :
    typedef typename promote_floating_point
        <
            typename select_calculation_type
                <
                    Point1,
                    Point2,
                    CalculationType
                >::type
        >::type calculation_type;

    inline haversine(calculation_type const& r = 1.0)
        : m_radius(r)
    {}


    static inline calculation_type apply(Point1 const& p1, Point2 const& p2)
    {
        return calculate(get_as_radian<0>(p1), get_as_radian<1>(p1),
                        get_as_radian<0>(p2), get_as_radian<1>(p2));
    }

    inline calculation_type radius() const
    {
        return m_radius;
    }


private :

    static inline calculation_type calculate(calculation_type const& lon1,
            calculation_type const& lat1,
            calculation_type const& lon2,
            calculation_type const& lat2)
    {
        return math::hav(lat2 - lat1)
                + cos(lat1) * cos(lat2) * math::hav(lon2 - lon1);
    }

    calculation_type m_radius;
};



} // namespace comparable

/*!
\brief Distance calculation for spherical coordinates
on a perfect sphere using haversine
\ingroup strategies
\tparam Point1 \tparam_first_point
\tparam Point2 \tparam_second_point
\tparam CalculationType \tparam_calculation
\author Adapted from: http://williams.best.vwh.net/avform.htm
\see http://en.wikipedia.org/wiki/Great-circle_distance
\note It says: <em>The great circle distance d between two
points with coordinates {lat1,lon1} and {lat2,lon2} is given by:
    d=acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon1-lon2))
A mathematically equivalent formula, which is less subject
    to rounding error for short distances is:
    d=2*asin(sqrt((sin((lat1-lat2)/2))^2
    + cos(lat1)*cos(lat2)*(sin((lon1-lon2)/2))^2))
    </em>

\qbk{
[heading See also]
[link geometry.reference.algorithms.distance.distance_3_with_strategy distance (with strategy)]
}

*/
template
<
    typename Point1,
    typename Point2 = Point1,
    typename CalculationType = void
>
class haversine
{
    typedef comparable::haversine<Point1, Point2, CalculationType> comparable_type;

public :

    typedef typename services::return_type<comparable_type>::type calculation_type;

    /*!
    \brief Constructor
    \param radius radius of the sphere, defaults to 1.0 for the unit sphere
    */
    inline haversine(calculation_type const& radius = 1.0)
        : m_radius(radius)
    {}

    /*!
    \brief applies the distance calculation
    \return the calculated distance (including multiplying with radius)
    \param p1 first point
    \param p2 second point
    */
    inline calculation_type apply(Point1 const& p1, Point2 const& p2) const
    {
        calculation_type const a = comparable_type::apply(p1, p2);
        calculation_type const c = calculation_type(2.0) * asin(sqrt(a));
        return m_radius * c;
    }

    /*!
    \brief access to radius value
    \return the radius
    */
    inline calculation_type radius() const
    {
        return m_radius;
    }

private :
    calculation_type m_radius;
};


#ifndef DOXYGEN_NO_STRATEGY_SPECIALIZATIONS
namespace services
{

template <typename Point1, typename Point2, typename CalculationType>
struct tag<haversine<Point1, Point2, CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct return_type<haversine<Point1, Point2, CalculationType> >
{
    typedef typename haversine<Point1, Point2, CalculationType>::calculation_type type;
};


template <typename Point1, typename Point2, typename CalculationType, typename P1, typename P2>
struct similar_type<haversine<Point1, Point2, CalculationType>, P1, P2>
{
    typedef haversine<P1, P2, CalculationType> type;
};


template <typename Point1, typename Point2, typename CalculationType, typename P1, typename P2>
struct get_similar<haversine<Point1, Point2, CalculationType>, P1, P2>
{
private :
    typedef haversine<Point1, Point2, CalculationType> this_type;
public :
    static inline typename similar_type<this_type, P1, P2>::type apply(this_type const& input)
    {
        return haversine<P1, P2, CalculationType>(input.radius());
    }
};

template <typename Point1, typename Point2, typename CalculationType>
struct comparable_type<haversine<Point1, Point2, CalculationType> >
{
    typedef comparable::haversine<Point1, Point2, CalculationType> type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct get_comparable<haversine<Point1, Point2, CalculationType> >
{
private :
    typedef haversine<Point1, Point2, CalculationType> this_type;
    typedef comparable::haversine<Point1, Point2, CalculationType> comparable_type;
public :
    static inline comparable_type apply(this_type const& input)
    {
        return comparable_type(input.radius());
    }
};

template <typename Point1, typename Point2, typename CalculationType>
struct result_from_distance<haversine<Point1, Point2, CalculationType> >
{
private :
    typedef haversine<Point1, Point2, CalculationType> this_type;
    typedef typename return_type<this_type>::type return_type;
public :
    template <typename T>
    static inline return_type apply(this_type const& , T const& value)
    {
        return return_type(value);
    }
};


// Specializations for comparable::haversine
template <typename Point1, typename Point2, typename CalculationType>
struct tag<comparable::haversine<Point1, Point2, CalculationType> >
{
    typedef strategy_tag_distance_point_point type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct return_type<comparable::haversine<Point1, Point2, CalculationType> >
{
    typedef typename comparable::haversine<Point1, Point2, CalculationType>::calculation_type type;
};


template <typename Point1, typename Point2, typename CalculationType, typename P1, typename P2>
struct similar_type<comparable::haversine<Point1, Point2, CalculationType>, P1, P2>
{
    typedef comparable::haversine<P1, P2, CalculationType> type;
};


template <typename Point1, typename Point2, typename CalculationType, typename P1, typename P2>
struct get_similar<comparable::haversine<Point1, Point2, CalculationType>, P1, P2>
{
private :
    typedef comparable::haversine<Point1, Point2, CalculationType> this_type;
public :
    static inline typename similar_type<this_type, P1, P2>::type apply(this_type const& input)
    {
        return comparable::haversine<P1, P2, CalculationType>(input.radius());
    }
};

template <typename Point1, typename Point2, typename CalculationType>
struct comparable_type<comparable::haversine<Point1, Point2, CalculationType> >
{
    typedef comparable::haversine<Point1, Point2, CalculationType> type;
};


template <typename Point1, typename Point2, typename CalculationType>
struct get_comparable<comparable::haversine<Point1, Point2, CalculationType> >
{
private :
    typedef comparable::haversine<Point1, Point2, CalculationType> this_type;
public :
    static inline this_type apply(this_type const& input)
    {
        return input;
    }
};


template <typename Point1, typename Point2, typename CalculationType>
struct result_from_distance<comparable::haversine<Point1, Point2, CalculationType> >
{
private :
    typedef comparable::haversine<Point1, Point2, CalculationType> strategy_type;
    typedef typename return_type<strategy_type>::type return_type;
public :
    template <typename T>
    static inline return_type apply(strategy_type const& strategy, T const& distance)
    {
        return_type const s = sin((distance / strategy.radius()) / return_type(2));
        return s * s;
    }
};


// Register it as the default for point-types 
// in a spherical equatorial coordinate system
template <typename Point1, typename Point2>
struct default_strategy<point_tag, Point1, Point2, spherical_equatorial_tag, spherical_equatorial_tag>
{
    typedef strategy::distance::haversine<Point1, Point2> type;
};

// Note: spherical polar coordinate system requires "get_as_radian_equatorial"


} // namespace services
#endif // DOXYGEN_NO_STRATEGY_SPECIALIZATIONS


}} // namespace strategy::distance


}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_STRATEGIES_SPHERICAL_DISTANCE_HAVERSINE_HPP
