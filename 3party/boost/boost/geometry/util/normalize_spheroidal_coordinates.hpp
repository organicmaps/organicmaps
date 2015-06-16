// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2015, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Licensed under the Boost Software License version 1.0.
// http://www.boost.org/users/license.html

#ifndef BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_COORDINATES_HPP
#define BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_COORDINATES_HPP

#include <boost/geometry/core/assert.hpp>
#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

namespace math 
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename CoordinateType, typename Units>
struct constants_on_spheroid
{
    static inline CoordinateType period()
    {
        return math::two_pi<CoordinateType>();
    }

    static inline CoordinateType half_period()
    {
        return math::pi<CoordinateType>();
    }

    static inline CoordinateType min_longitude()
    {
        static CoordinateType const minus_pi = -math::pi<CoordinateType>();
        return minus_pi;
    }

    static inline CoordinateType max_longitude()
    {
        return math::pi<CoordinateType>();
    }

    static inline CoordinateType min_latitude()
    {
        static CoordinateType const minus_half_pi
            = -math::half_pi<CoordinateType>();
        return minus_half_pi;
    }

    static inline CoordinateType max_latitude()
    {
        return math::half_pi<CoordinateType>();
    }
};

template <typename CoordinateType>
struct constants_on_spheroid<CoordinateType, degree>
{
    static inline CoordinateType period()
    {
        return CoordinateType(360.0);
    }

    static inline CoordinateType half_period()
    {
        return CoordinateType(180.0);
    }

    static inline CoordinateType min_longitude()
    {
        return CoordinateType(-180.0);
    }

    static inline CoordinateType max_longitude()
    {
        return CoordinateType(180.0);
    }

    static inline CoordinateType min_latitude()
    {
        return CoordinateType(-90.0);
    }

    static inline CoordinateType max_latitude()
    {
        return CoordinateType(90.0);
    }
};


template <typename Units, typename CoordinateType>
class normalize_spheroidal_coordinates
{
    typedef constants_on_spheroid<CoordinateType, Units> constants;

protected:
    static inline CoordinateType normalize_up(CoordinateType const& value)
    {
        return
            math::mod(value + constants::half_period(), constants::period())
            - constants::half_period();            
    }

    static inline CoordinateType normalize_down(CoordinateType const& value)
    {
        return
            math::mod(value - constants::half_period(), constants::period())
            + constants::half_period();            
    }

public:
    static inline void apply(CoordinateType& longitude,
                             CoordinateType& latitude,
                             bool normalize_poles = true)
    {
#ifdef BOOST_GEOMETRY_NORMALIZE_LATITUDE
        // normalize latitude
        if (math::larger(latitude, constants::half_period()))
        {
            latitude = normalize_up(latitude);
        }
        else if (math::smaller(latitude, -constants::half_period()))
        {
            latitude = normalize_down(latitude);
        }

        // fix latitude range
        if (latitude < constants::min_latitude())
        {
            latitude = -constants::half_period() - latitude;
            longitude -= constants::half_period();
        }
        else if (latitude > constants::max_latitude())
        {
            latitude = constants::half_period() - latitude;
            longitude -= constants::half_period();
        }
#endif // BOOST_GEOMETRY_NORMALIZE_LATITUDE

        // normalize longitude
        if (math::equals(math::abs(longitude), constants::half_period()))
        {
            longitude = constants::half_period();
        }
        else if (longitude > constants::half_period())
        {
            longitude = normalize_up(longitude);
            if (math::equals(longitude, -constants::half_period()))
            {
                longitude = constants::half_period();
            }
        }
        else if (longitude < -constants::half_period())
        {
            longitude = normalize_down(longitude);
        }

        // finally normalize poles
        if (normalize_poles)
        {
            if (math::equals(math::abs(latitude), constants::max_latitude()))
            {
                // for the north and south pole we set the longitude to 0
                // (works for both radians and degrees)
                longitude = CoordinateType(0);
            }
        }

#ifdef BOOST_GEOMETRY_NORMALIZE_LATITUDE
        BOOST_GEOMETRY_ASSERT(! math::larger(constants::min_latitude(), latitude));
        BOOST_GEOMETRY_ASSERT(! math::larger(latitude, constants::max_latitude()));
#endif // BOOST_GEOMETRY_NORMALIZE_LATITUDE

        BOOST_GEOMETRY_ASSERT(math::smaller(constants::min_longitude(), longitude));
        BOOST_GEOMETRY_ASSERT(! math::larger(longitude, constants::max_longitude()));
    }
};


} // namespace detail
#endif // DOXYGEN_NO_DETAIL


/*!
\brief Short utility to normalize the coordinates on a spheroid
\tparam Units The units of the coordindate system in the spheroid
\tparam CoordinateType The type of the coordinates
\param longitude Longitude
\param latitude Latitude
\ingroup utility
*/
template <typename Units, typename CoordinateType>
inline void normalize_spheroidal_coordinates(CoordinateType& longitude,
                                             CoordinateType& latitude)
{
    detail::normalize_spheroidal_coordinates
        <
            Units, CoordinateType
        >::apply(longitude, latitude);
}


} // namespace math


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_NORMALIZE_SPHEROIDAL_COORDINATES_HPP
