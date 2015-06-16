// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_GIS_LATLONG_POINT_LL_HPP
#define BOOST_GEOMETRY_EXTENSIONS_GIS_LATLONG_POINT_LL_HPP

#include <cstddef>
#include <sstream>
#include <string>

#include <boost/numeric/conversion/cast.hpp>

#include <boost/geometry/core/cs.hpp>
#include <boost/geometry/arithmetic/arithmetic.hpp>
#include <boost/geometry/geometries/point.hpp>

#include <boost/geometry/extensions/gis/latlong/detail/graticule.hpp>

namespace boost { namespace geometry
{


namespace model { namespace ll
{

/*!
    \brief Point using spherical coordinates \a lat and \a lon, on Earth
    \ingroup Geometry
    \details The point_ll class implements a point with lat and lon functions.
    It can be constructed using latitude and longitude classes. The latlong
    class can be defined in degrees or in radians. There is a conversion method
    from degree to radian, and from radian to degree.
    \tparam Units units,defaults to degree
    \tparam CoordinateType coordinate type, double (the default) or float
        (it might be int as well)
    \tparam CoordinateSystem coordinate system, should include NOT degree/radian
        indication, should be e.g. cs::geographic or cs::spherical
    \tparam Dimensions number of dimensions
    \note There is NO constructor with two values to avoid
        exchanging lat and long
    \note Construction with latitude and longitude can be done in both orders,
        so lat/long and long/lat
    \par Example:
    Example showing how the point_ll class can be constructed. Note that it
        can also be constructed using
    decimal degrees (43.123).
    \dontinclude doxygen_1.cpp
    \skip example_point_ll_construct
    \line {
    \until }
*/
template
<
    typename Units = degree,
    typename CoordinateType = double,
    template<typename> class CoordinateSystem = cs::geographic,
    std::size_t Dimensions = 2
>
class point : public model::point
                <
                    CoordinateType,
                    Dimensions,
                    CoordinateSystem<Units>
                >
{
    typedef model::point
        <
            CoordinateType,
            Dimensions,
            CoordinateSystem<Units>
        >
        base_type;
public:

    /// Default constructor, does not initialize anything
    inline point() : base_type() {}

    /// Constructor with longitude/latitude
    inline point(longitude<CoordinateType> const& lo,
                latitude<CoordinateType> const& la)
        : base_type(lo, la) {}

    /// Constructor with latitude/longitude
    inline point(latitude<CoordinateType> const& la,
                longitude<CoordinateType> const& lo)
        : base_type(lo, la) {}

    /// Get longitude
    inline CoordinateType const& lon() const
    { return this->template get<0>(); }

    /// Get latitude
    inline CoordinateType const& lat() const
    { return this->template get<1>(); }

    /// Set longitude
    inline void lon(CoordinateType const& v)
    { this->template set<0>(v); }

    /// Set latitude
    inline void lat(CoordinateType const& v)
    { this->template set<1>(v); }

    /// Set longitude using dms class
    inline void lon(dms<east, CoordinateType> const& v)
    {
        this->template set<0>(v.as_value());
    }
    inline void lon(dms<west, CoordinateType> const& v)
    {
        this->template set<0>(v.as_value());
    }

    inline void lat(dms<north, CoordinateType> const& v)
    {
        this->template set<1>(v.as_value());
    }
    inline void lat(dms<south, CoordinateType> const& v)
    {
        this->template set<1>(v.as_value());
    }
};


}} // namespace model::ll

// Adapt the point_ll to the concept
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template
<
    typename Units,
    typename CoordinateType,
    template<typename> class CoordinateSystem,
    std::size_t DimensionCount
>
struct tag
        <
            model::ll::point
            <
                Units,
                CoordinateType,
                CoordinateSystem,
                DimensionCount
            >
        >
{
    typedef point_tag type;
};

template
<
    typename Units,
    typename CoordinateType,
    template<typename> class CoordinateSystem,
    std::size_t DimensionCount
>
struct coordinate_type
        <
            model::ll::point
            <
                Units,
                CoordinateType,
                CoordinateSystem,
                DimensionCount
            >
        >
{
    typedef CoordinateType type;
};

template
<
    typename Units,
    typename CoordinateType,
    template<typename> class CoordinateSystem,
    std::size_t DimensionCount
>
struct coordinate_system
        <
            model::ll::point
            <
                Units,
                CoordinateType,
                CoordinateSystem,
                DimensionCount
            >
        >
{
    typedef CoordinateSystem<Units> type;
};

template
<
    typename Units,
    typename CoordinateType,
    template<typename> class CoordinateSystem,
    std::size_t DimensionCount
>
struct dimension
        <
            model::ll::point
            <
                Units,
                CoordinateType,
                CoordinateSystem,
                DimensionCount
            >
        >
    : boost::mpl::int_<DimensionCount>
{};

template
<
    typename Units,
    typename CoordinateType,
    template<typename> class CoordinateSystem,
    std::size_t DimensionCount,
    std::size_t Dimension
>
struct access
        <
            model::ll::point
            <
                Units,
                CoordinateType,
                CoordinateSystem,
                DimensionCount
            >,
            Dimension
        >
{
    typedef model::ll::point
            <
                Units,
                CoordinateType,
                CoordinateSystem,
                DimensionCount
            > type;

    static inline CoordinateType get(type const& p)
    {
        return p.template get<Dimension>();
    }

    static inline void set(type& p, CoordinateType const& value)
    {
        p.template set<Dimension>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_GIS_LATLONG_POINT_LL_HPP
