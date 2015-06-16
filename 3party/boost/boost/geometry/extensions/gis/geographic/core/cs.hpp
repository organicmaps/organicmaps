// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_GIS_GEOGRAPHIC_CORE_CS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_GIS_GEOGRAPHIC_CORE_CS_HPP



namespace boost { namespace geometry
{

namespace cs
{

/*!
    \brief EPSG Cartesian coordinate system
    \details EPSG (European Petrol Survey Group) has a standard list of projections,
        each having a code
    \see
    \ingroup cs
    \tparam Code the EPSG code
    \todo Maybe derive from boost::mpl::int_<EpsgCode>
*/
template<std::size_t Code>
struct epsg
{
    static const std::size_t epsg_code = Code;
};



/*!
    \brief Earth Centered, Earth Fixed
    \details Defines a Cartesian coordinate system x,y,z with the center of the earth as its origin,
        going through the Greenwich
    \see http://en.wikipedia.org/wiki/ECEF
    \see http://en.wikipedia.org/wiki/Geodetic_system
    \note Also known as "Geocentric", but geocentric is also an astronomic coordinate system
    \ingroup cs
*/
struct ecef
{
};


} // namespace cs

namespace traits
{

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS

template<>
struct cs_tag<cs::ecef>
{
    typedef cartesian_tag type;
};

template <std::size_t C>
struct cs_tag<cs::epsg<C> >
{
    typedef cartesian_tag type;
};

#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS
} // namespace traits


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_GIS_GEOGRAPHIC_CORE_CS_HPP
