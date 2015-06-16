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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_MATRIX_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_MATRIX_HPP

#include <cstddef>

#include <boost/geometry/extensions/algebra/core/tags.hpp>
#include <boost/geometry/extensions/algebra/geometries/concepts/matrix_concept.hpp>

namespace boost { namespace geometry {

namespace model {

template <typename T, std::size_t Rows, std::size_t Cols>
class matrix
{
    BOOST_CONCEPT_ASSERT( (concept::Matrix<matrix>) );

public:

    /// @brief Default constructor, no initialization
    inline matrix()
    {}

    /// @brief Get a coordinate
    /// @tparam I row index
    /// @tparam J col index
    /// @return the cell value
    template <std::size_t I, std::size_t J>
    inline T const& get() const
    {
        BOOST_STATIC_ASSERT(I < Rows);
        BOOST_STATIC_ASSERT(J < Cols);
        return m_values[I + Rows * J];
    }

    /// @brief Set a coordinate
    /// @tparam I row index
    /// @tparam J col index
    /// @param value value to set
    template <std::size_t I, std::size_t J>
    inline void set(T const& value)
    {
        BOOST_STATIC_ASSERT(I < Rows);
        BOOST_STATIC_ASSERT(J < Cols);
        m_values[I + Rows * J] = value;
    }

private:

    T m_values[Rows * Cols];
};

} // namespace model

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename CoordinateType, std::size_t Rows, std::size_t Cols>
struct tag<model::matrix<CoordinateType, Rows, Cols> >
{
    typedef matrix_tag type;
};

template <typename CoordinateType, std::size_t Rows, std::size_t Cols>
struct coordinate_type<model::matrix<CoordinateType, Rows, Cols> >
{
    typedef CoordinateType type;
};

//template <typename CoordinateType, std::size_t Rows, std::size_t Cols>
//struct coordinate_system<model::matrix<CoordinateType, Rows, Cols> >
//{
//    typedef cs::cartesian type;
//};

// TODO - move this class to traits.hpp
template <typename Geometry, std::size_t Index>
struct indexed_dimension
{
     BOOST_MPL_ASSERT_MSG(false,
                          NOT_IMPLEMENTED_FOR_THIS_GEOMETRY_OR_INDEX,
                          (Geometry, boost::integral_constant<std::size_t, Index>));
};

template <typename CoordinateType, std::size_t Rows, std::size_t Cols>
struct indexed_dimension<model::matrix<CoordinateType, Rows, Cols>, 0>
    : boost::integral_constant<std::size_t, Rows>
{};

template <typename CoordinateType, std::size_t Rows, std::size_t Cols>
struct indexed_dimension<model::matrix<CoordinateType, Rows, Cols>, 1>
    : boost::integral_constant<std::size_t, Cols>
{};

template <typename CoordinateType, std::size_t Rows, std::size_t Cols, std::size_t I, std::size_t J>
struct indexed_access<model::matrix<CoordinateType, Dimension>, I, J>
{
    typedef CoordinateType coordinate_type;

    static inline coordinate_type get(model::matrix<CoordinateType, Rows, Cols> const& m)
    {
        return m.template get<I, J>();
    }

    static inline void set(model::matrix<CoordinateType, Rows, Cols> & m, coordinate_type const& value)
    {
        m.template set<I, J>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_MATRIX_HPP
