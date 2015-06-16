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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_ACCESS_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_ACCESS_HPP



#include <boost/geometry/core/access.hpp>

#include <boost/geometry/extensions/algebra/core/tags.hpp>


namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch
{

template <typename Vector, typename CoordinateType, std::size_t Dimension>
struct access<vector_tag, Vector, CoordinateType, Dimension, boost::false_type>
{
    static inline CoordinateType get(Vector const& v)
    {
        return traits::access<Vector, Dimension>::get(v);
    }
    static inline void set(Vector& v, CoordinateType const& value)
    {
        traits::access<Vector, Dimension>::set(v, value);
    }
};

template <typename Vector, typename CoordinateType, std::size_t Dimension>
struct access<vector_tag, Vector, CoordinateType, Dimension, boost::true_type>
{
    static inline CoordinateType get(Vector const* v)
    {
        return traits::access<typename boost::remove_pointer<Vector>::type, Dimension>::get(*v);
    }
    static inline void set(Vector* v, CoordinateType const& value)
    {
        traits::access<typename boost::remove_pointer<Vector>::type, Dimension>::set(*v, value);
    }
};

template <typename Q, typename CoordinateType, std::size_t Dimension>
struct access<quaternion_tag, Q, CoordinateType, Dimension, boost::false_type>
{
    static inline CoordinateType get(Q const& v)
    {
        return traits::access<Q, Dimension>::get(v);
    }
    static inline void set(Q& v, CoordinateType const& value)
    {
        traits::access<Q, Dimension>::set(v, value);
    }
};

template <typename Q, typename CoordinateType, std::size_t Dimension>
struct access<quaternion_tag, Q, CoordinateType, Dimension, boost::true_type>
{
    static inline CoordinateType get(Q const* v)
    {
        return traits::access<typename boost::remove_pointer<Q>::type, Dimension>::get(*v);
    }
    static inline void set(Q* v, CoordinateType const& value)
    {
        traits::access<typename boost::remove_pointer<Q>::type, Dimension>::set(*v, value);
    }
};

template<typename M, typename CoordinateType, std::size_t I, std::size_t J>
struct indexed_access<matrix_tag, M, CoordinateType, I, J, boost::false_type>
    : detail::indexed_access_non_pointer<M, CoordinateType, I, J>
{};

template<typename M, typename CoordinateType, std::size_t I, std::size_t J>
struct indexed_access<matrix_tag, M, CoordinateType, I, J, boost::true_type>
    : detail::indexed_access_pointer<M, CoordinateType, I, J>
{};


template <typename Q, typename CoordinateType, std::size_t Dimension>
struct access<rotation_quaternion_tag, Q, CoordinateType, Dimension, boost::false_type>
{
    static inline CoordinateType get(Q const& v)
    {
        return traits::access<Q, Dimension>::get(v);
    }
    static inline void set(Q& v, CoordinateType const& value)
    {
        traits::access<Q, Dimension>::set(v, value);
    }
};

template <typename Q, typename CoordinateType, std::size_t Dimension>
struct access<rotation_quaternion_tag, Q, CoordinateType, Dimension, boost::true_type>
{
    static inline CoordinateType get(Q const* v)
    {
        return traits::access<typename boost::remove_pointer<Q>::type, Dimension>::get(*v);
    }
    static inline void set(Q* v, CoordinateType const& value)
    {
        traits::access<typename boost::remove_pointer<Q>::type, Dimension>::set(*v, value);
    }
};

template<typename RM, typename CoordinateType, std::size_t I, std::size_t J>
struct indexed_access<rotation_matrix_tag, RM, CoordinateType, I, J, boost::false_type>
    : detail::indexed_access_non_pointer<RM, CoordinateType, I, J>
{};

template<typename RM, typename CoordinateType, std::size_t I, std::size_t J>
struct indexed_access<rotation_matrix_tag, RM, CoordinateType, I, J, boost::true_type>
    : detail::indexed_access_pointer<RM, CoordinateType, I, J>
{};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_ACCESS_HPP
