// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_GIS_LATLONG_DETAIL_GRATICULE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_GIS_LATLONG_DETAIL_GRATICULE_HPP

#include <cmath>
#include <sstream>
#include <string>

#include <boost/numeric/conversion/cast.hpp>
#include <boost/geometry/util/math.hpp>


namespace boost { namespace geometry
{

/*!
    \brief Cardinal directions.
    \ingroup cs
    \details They are used in the dms-class. When specified by the library user,
    north/east/south/west is, in general, enough. When parsed or received by an algorithm,
    the user knows it it is lat/long but not more
*/
enum cd_selector
{
    /*cd_none, */
    north,
    east,
    south,
    west,
    cd_lat,
    cd_lon
};

/*!
    \brief Utility class to assign poinst with degree,minute,second
    \ingroup cs
    \note Normally combined with latitude and longitude classes
    \tparam CardinalDir selects if it is north/south/west/east
    \tparam coordinate value, double/float
    \par Example:
    Example showing how to use the dms class
    \dontinclude doxygen_1.cpp
    \skip example_dms
    \line {
    \until }
*/
template <cd_selector CardinalDir, typename T = double>
class dms
{
public:

    /// Constructs with a value
    inline explicit dms(T v)
        : m_value(v)
    {}

    /// Constructs with a degree, minute, optional second
    inline explicit dms(int d, int m, T s = 0.0)
    {
        double v = ((CardinalDir == west || CardinalDir == south) ? -1.0 : 1.0)
                    * (double(d) + (m / 60.0) + (s / 3600.0));

        m_value = boost::numeric_cast<T>(v);
    }

    // Prohibit automatic conversion to T
    // because this would enable lon(dms<south>)
    // inline operator T() const { return m_value; }

    /// Explicit conversion to T (double/float)
    inline const T& as_value() const
    {
        return m_value;
    }

    /// Get degrees as integer, minutes as integer, seconds as double.
    inline void get_dms(int& d, int& m, double& s,
                        bool& positive, char& cardinal) const
    {
        double value = m_value;

        // Set to normal earth latlong coordinates
        while (value < -180)
        {
            value += 360;
        }
        while (value > 180)
        {
            value -= 360;
        }
        // Make positive and indicate this
        positive = value > 0;

        // Todo: we might implement template/specializations here
        // Todo: if it is "west" and "positive", make east? or keep minus sign then?

        cardinal = ((CardinalDir == cd_lat && positive) ? 'N'
            :  (CardinalDir == cd_lat && !positive) ? 'S'
            :  (CardinalDir == cd_lon && positive) ? 'E'
            :  (CardinalDir == cd_lon && !positive) ? 'W'
            :  (CardinalDir == east) ? 'E'
            :  (CardinalDir == west) ? 'W'
            :  (CardinalDir == north) ? 'N'
            :  (CardinalDir == south) ? 'S'
            : ' ');

        value = geometry::math::abs(value);

        // Calculate the values
        double fraction = 0;
        double integer = 0;
        fraction = std::modf(value, &integer);
        d = int(integer);
        s = 60.0 * std::modf(fraction * 60.0, &integer);
        m = int(integer);
    }

    /// Get degrees, minutes, seconds as a string, separators can be specified optionally
    inline std::string get_dms(std::string const& ds = " ",
        const std::string& ms = "'",
        const std::string& ss = "\"") const
    {
        double s = 0;
        int d = 0;
        int m = 0;
        bool positive = false;
        char cardinal = 0;
        get_dms(d, m, s, positive, cardinal);
        std::ostringstream out;
        out << d << ds << m << ms << s << ss << " " << cardinal;

        return out.str();
    }

private:

    T m_value;
};


#ifndef DOXYGEN_NO_DETAIL
namespace detail
{
/*!
    \brief internal base class for latitude and longitude classes
    \details The latitude longitude classes define different types for lat and lon. This is convenient
    to construct latlong class without ambiguity.
    \note It is called graticule, after <em>"This latitude/longitude "webbing" is known as the common
    graticule" (http://en.wikipedia.org/wiki/Geographic_coordinate_system)</em>
    \tparam S latitude/longitude
    \tparam T coordinate type, double float or int
*/
template <typename T>
class graticule
{
public:

    // TODO: Pass 'v' by const-ref
    inline explicit graticule(T v) : m_v(v) {}
    inline operator T() const { return m_v; }

private:

    T m_v;
};

} // namespace detail
#endif // DOXYGEN_NO_DETAIL

/*!
    \brief Utility class to assign points with latitude value (north/south)
    \ingroup cs
    \tparam T coordinate type, double / float
    \note Often combined with dms class
*/
template <typename T = double>
class latitude : public detail::graticule<T>
{
public:

    /// Can be constructed with a value
    inline explicit latitude(T v)
        : detail::graticule<T>(v)
    {}

    /// Can be constructed with a NORTH dms-class
    inline explicit latitude(dms<north,T> const& v)
        : detail::graticule<T>(v.as_value())
    {}

    /// Can be constructed with a SOUTH dms-class
    inline explicit latitude(dms<south,T> const& v)
       : detail::graticule<T>(v.as_value())
   {}
};

/*!
\brief Utility class to assign points with longitude value (west/east)
\ingroup cs
\tparam T coordinate type, double / float
\note Often combined with dms class
*/
template <typename T = double>
class longitude : public detail::graticule<T>
{
public:

    /// Can be constructed with a value
    inline explicit longitude(T v)
        : detail::graticule<T>(v)
    {}

    /// Can be constructed with a WEST dms-class
    inline explicit longitude(dms<west, T> const& v)
        : detail::graticule<T>(v.as_value())
    {}

    /// Can be constructed with an EAST dms-class
    inline explicit longitude(dms<east, T> const& v)
        : detail::graticule<T>(v.as_value())
    {}
};

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_GIS_LATLONG_DETAIL_GRATICULE_HPP
