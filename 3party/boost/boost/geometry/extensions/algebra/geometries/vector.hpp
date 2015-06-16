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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_VECTOR_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_VECTOR_HPP

#include <cstddef>

#include <boost/geometry/algorithms/assign.hpp>
#include <boost/geometry/algorithms/convert.hpp>

#include <boost/geometry/extensions/algebra/core/tags.hpp>
#include <boost/geometry/extensions/algebra/geometries/concepts/vector_concept.hpp>

namespace boost { namespace geometry
{

namespace model
{

template <typename CoordinateType, std::size_t DimensionCount>
class vector
{
    BOOST_CONCEPT_ASSERT( (concept::Vector<vector>) );

public:

    /// @brief Default constructor, no initialization
    inline vector()
    {}

    /// @brief Constructor to set one, two or three values
    inline explicit vector(CoordinateType const& v0, CoordinateType const& v1 = 0, CoordinateType const& v2 = 0)
    {
        if (DimensionCount >= 1) m_values[0] = v0;
        if (DimensionCount >= 2) m_values[1] = v1;
        if (DimensionCount >= 3) m_values[2] = v2;
    }

    /// @brief Get a coordinate
    /// @tparam K coordinate to get
    /// @return the coordinate
    template <std::size_t K>
    inline CoordinateType const& get() const
    {
        BOOST_STATIC_ASSERT(K < DimensionCount);
        return m_values[K];
    }

    /// @brief Set a coordinate
    /// @tparam K coordinate to set
    /// @param value value to set
    template <std::size_t K>
    inline void set(CoordinateType const& value)
    {
        BOOST_STATIC_ASSERT(K < DimensionCount);
        m_values[K] = value;
    }

private:

    CoordinateType m_values[DimensionCount];
};


} // namespace model

#ifndef DOXYGEN_NO_TRAITS_SPECIALIZATIONS
namespace traits
{

template <typename CoordinateType, std::size_t DimensionCount>
struct tag<model::vector<CoordinateType, DimensionCount> >
{
    typedef vector_tag type;
};

template <typename CoordinateType, std::size_t DimensionCount>
struct coordinate_type<model::vector<CoordinateType, DimensionCount> >
{
    typedef CoordinateType type;
};

template <typename CoordinateType, std::size_t DimensionCount>
struct coordinate_system<model::vector<CoordinateType, DimensionCount> >
{
    typedef cs::cartesian type;
};

template <typename CoordinateType, std::size_t DimensionCount>
struct dimension<model::vector<CoordinateType, DimensionCount> >
    : boost::mpl::int_<DimensionCount>
{};

template
<
    typename CoordinateType,
    std::size_t DimensionCount,
    std::size_t Dimension
>
struct access<model::vector<CoordinateType, DimensionCount>, Dimension>
{
    static inline CoordinateType get(
        model::vector<CoordinateType, DimensionCount> const& v)
    {
        return v.template get<Dimension>();
    }

    static inline void set(
        model::vector<CoordinateType, DimensionCount> & v,
        CoordinateType const& value)
    {
        v.template set<Dimension>(value);
    }
};

} // namespace traits
#endif // DOXYGEN_NO_TRAITS_SPECIALIZATIONS

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_VECTOR_HPP
