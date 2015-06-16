// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.
// Copyright (c) 2013 Adam Wulkiewicz, Lodz, Poland.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_QUATERNION_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_QUATERNION_HPP

#include <cstddef>

#include <boost/geometry/extensions/algebra/core/tags.hpp>
#include <boost/geometry/extensions/algebra/geometries/concepts/quaternion_concept.hpp>

// WARNING!
// It is probable that the sequence of coordinate will change in the future
// at the beginning there would be xyz, w would become the last coordinate

namespace boost { namespace geometry
{

namespace model
{

template <typename T>
class quaternion
{
    BOOST_CONCEPT_ASSERT( (concept::Quaternion<quaternion>) );

public:

    /// @brief Default constructor, no initialization
    inline quaternion()
    {}

    /// @brief Constructor to set components
    inline quaternion(T const& w, T const& x, T const& y, T const& z)
    {
        m_values[0] = w;
        m_values[1] = x;
        m_values[2] = y;
        m_values[3] = z;
    }

    /// @brief Get a coordinate
    /// @tparam K coordinate to get
    /// @return the coordinate
    template <std::size_t K>
    inline T const& get() const
    {
        BOOST_STATIC_ASSERT(K < 4);
        return m_values[K];
    }

    /// @brief Set a coordinate
    /// @tparam K coordinate to set
    /// @param value value to set
    template <std::size_t K>
    inline void set(T const& value)
    {
        BOOST_STATIC_ASSERT(K < 4);
        m_values[K] = value;
    }

private:

    T m_values[4];
};


} // namespace model

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename CoordinateType>
struct tag<model::quaternion<CoordinateType> >
{
    typedef quaternion_tag type;
};

template <typename CoordinateType>
struct coordinate_type<model::quaternion<CoordinateType> >
{
    typedef CoordinateType type;
};

//template <typename CoordinateType>
//struct coordinate_system<model::quaternion<CoordinateType> >
//{
//    typedef cs::cartesian type;
//};

template <typename CoordinateType>
struct dimension<model::quaternion<CoordinateType> >
    : boost::integral_constant<std::size_t, 4>
{};

template<typename CoordinateType, std::size_t Dimension>
struct access<model::quaternion<CoordinateType>, Dimension>
{
    static inline CoordinateType get(
        model::quaternion<CoordinateType> const& v)
    {
        return v.template get<Dimension>();
    }

    static inline void set(
        model::quaternion<CoordinateType> & v,
        CoordinateType const& value)
    {
        v.template set<Dimension>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_QUATERNION_HPP
