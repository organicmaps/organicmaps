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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_ROTATION_MATRIX_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_ROTATION_MATRIX_HPP

#include <cstddef>

#include <boost/geometry/extensions/algebra/core/tags.hpp>
#include <boost/geometry/extensions/algebra/geometries/concepts/rotation_matrix_concept.hpp>

namespace boost { namespace geometry {

namespace model {

template <typename T, std::size_t Dimension>
class rotation_matrix
{
    BOOST_CONCEPT_ASSERT( (concept::RotationMatrix<rotation_matrix>) );

public:

    /// @brief Default constructor, no initialization
    inline rotation_matrix()
    {}

    /// @brief Get a coordinate
    /// @tparam I row index
    /// @tparam J col index
    /// @return the cell value
    template <std::size_t I, std::size_t J>
    inline T const& get() const
    {
        BOOST_STATIC_ASSERT(I < Dimension);
        BOOST_STATIC_ASSERT(J < Dimension);
        return m_values[I * Dimension + J];
    }

    /// @brief Set a coordinate
    /// @tparam I row index
    /// @tparam J col index
    /// @param value value to set
    template <std::size_t I, std::size_t J>
    inline void set(T const& value)
    {
        BOOST_STATIC_ASSERT(I < Dimension);
        BOOST_STATIC_ASSERT(J < Dimension);
        m_values[I * Dimension + J] = value;
    }

private:

    T m_values[Dimension * Dimension];
};

} // namespace model

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename CoordinateType, std::size_t Dimension>
struct tag<model::rotation_matrix<CoordinateType, Dimension> >
{
    typedef rotation_matrix_tag type;
};

template <typename CoordinateType, std::size_t Dimension>
struct coordinate_type<model::rotation_matrix<CoordinateType, Dimension> >
{
    typedef CoordinateType type;
};

template <typename CoordinateType, std::size_t Dimension>
struct coordinate_system<model::rotation_matrix<CoordinateType, Dimension> >
{
    typedef cs::cartesian type;
};

template <typename CoordinateType, std::size_t Dimension>
struct dimension<model::rotation_matrix<CoordinateType, Dimension> >
    : boost::mpl::int_<Dimension>
{};

template <typename CoordinateType, std::size_t Dimension, std::size_t I, std::size_t J>
struct indexed_access<model::rotation_matrix<CoordinateType, Dimension>, I, J>
{
    typedef CoordinateType coordinate_type;

    static inline coordinate_type get(model::rotation_matrix<CoordinateType, Dimension> const& m)
    {
        return m.template get<I, J>();
    }

    static inline void set(model::rotation_matrix<CoordinateType, Dimension> & m, coordinate_type const& value)
    {
        m.template set<I, J>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_ROTATION_MATRIX_HPP
