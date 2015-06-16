// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_NSPHERE_GEOMETRIES_NSPHERE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_NSPHERE_GEOMETRIES_NSPHERE_HPP

#include <cstddef>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/convert.hpp>
#include <boost/geometry/geometries/concepts/point_concept.hpp>


#include <boost/geometry/extensions/nsphere/core/tags.hpp>
#include <boost/geometry/extensions/nsphere/algorithms/assign.hpp>
#include <boost/geometry/extensions/nsphere/geometries/concepts/nsphere_concept.hpp>

namespace boost { namespace geometry
{

namespace model
{


/*!
    \brief Class nsphere: defines a circle or a sphere: a point with radius
    \ingroup Geometry
    \details The name nsphere is quite funny but the best description of the class. It can be a circle (2D),
    a sphere (3D), or higher (hypersphere) or lower. According to Wikipedia this name is the most appropriate.
    It was mentioned on the Boost list.
    An alternative is the more fancy name "sphercle" but that might be a bit too much an invention.
    \note Circle is currently used for selections, for example polygon_in_circle. Currently not all
    algorithms are implemented for n-spheres.
    \tparam P point type of the center
    \tparam T number type of the radius
 */
template <typename P, typename T>
class nsphere
{
    BOOST_CONCEPT_ASSERT( (concept::Point<P>) );

public:

    typedef T radius_type;
    typedef typename coordinate_type<P>::type coordinate_type;

    nsphere()
        : m_radius(0)
    {
        assign_value(m_center, coordinate_type());
    }

    nsphere(P const& center, T const& radius)
        : m_radius(radius)
    {
        geometry::convert(center, m_center);
    }

    inline P const& center() const { return m_center; }
    inline T const& radius() const { return m_radius; }

    inline void radius(T const& r) { m_radius = r; }
    inline P& center() { return m_center; }

private:

    P m_center;
    T m_radius;
};


} // namespace model

// Traits specializations for n-sphere above
#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename Point, typename RadiusType>
struct tag<model::nsphere<Point, RadiusType> >
{
    typedef nsphere_tag type;
};

template <typename Point, typename RadiusType>
struct point_type<model::nsphere<Point, RadiusType> >
{
    typedef Point type;
};

template <typename Point, typename RadiusType>
struct radius_type<model::nsphere<Point, RadiusType> >
{
    typedef RadiusType type;
};

template <typename Point, typename CoordinateType, std::size_t Dimension>
struct access<model::nsphere<Point, CoordinateType>,  Dimension>
{
    typedef model::nsphere<Point, CoordinateType> nsphere_type;

    static inline CoordinateType get(nsphere_type const& s)
    {
        return geometry::get<Dimension>(s.center());
    }

    static inline void set(nsphere_type& s, CoordinateType const& value)
    {
        geometry::set<Dimension>(s.center(), value);
    }
};

template <typename Point, typename RadiusType>
struct radius_access<model::nsphere<Point, RadiusType>, 0>
{
    typedef model::nsphere<Point, RadiusType> nsphere_type;

    static inline RadiusType get(nsphere_type const& s)
    {
        return s.radius();
    }

    static inline void set(nsphere_type& s, RadiusType const& value)
    {
        s.radius(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_NSPHERE_GEOMETRIES_NSPHERE_HPP
