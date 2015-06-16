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


#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_CONCEPTS_CHECK_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_CONCEPTS_CHECK_HPP


#include <boost/geometry/geometries/concepts/check.hpp>


namespace boost { namespace geometry
{


#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

template <typename Geometry>
struct check<Geometry, vector_tag, true>
    : detail::concept_check::check<concept::ConstVector<Geometry> >
{};

template <typename Geometry>
struct check<Geometry, vector_tag, false>
    : detail::concept_check::check<concept::Vector<Geometry> >
{};

template <typename Geometry>
struct check<Geometry, rotation_quaternion_tag, true>
    : detail::concept_check::check<concept::ConstRotationQuaternion<Geometry> >
{};

template <typename Geometry>
struct check<Geometry, rotation_quaternion_tag, false>
    : detail::concept_check::check<concept::RotationQuaternion<Geometry> >
{};

template <typename Geometry>
struct check<Geometry, rotation_matrix_tag, true>
    : detail::concept_check::check<concept::ConstRotationMatrix<Geometry> >
{};

template <typename Geometry>
struct check<Geometry, rotation_matrix_tag, false>
    : detail::concept_check::check<concept::RotationMatrix<Geometry> >
{};

} // namespace dispatch
#endif




}} // namespace boost::geometry


#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_GEOMETRIES_CONCEPTS_CHECK_HPP
