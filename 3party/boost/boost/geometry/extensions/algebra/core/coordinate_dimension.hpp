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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_COORDINATE_DIMENSION_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_COORDINATE_DIMENSION_HPP

#include <boost/geometry/core/coordinate_system.hpp>

#include <boost/geometry/extensions/algebra/core/tags.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DISPATCH
namespace core_dispatch {

template <typename V>
struct dimension<vector_tag, V>
    : traits::dimension<typename geometry::util::bare_type<V>::type>
{};

template <typename G>
struct dimension<quaternion_tag, G>
    : traits::dimension<typename geometry::util::bare_type<G>::type>
{};

template <typename G, std::size_t Index>
struct indexed_dimension<matrix_tag, G, Index>
    : traits::indexed_dimension<typename geometry::util::bare_type<G>::type, Index>
{};


template <typename G>
struct dimension<rotation_quaternion_tag, G>
    : traits::dimension<typename geometry::util::bare_type<G>::type>
{};

template <typename G>
struct dimension<rotation_matrix_tag, G>
    : traits::dimension<typename geometry::util::bare_type<G>::type>
{};

} // namespace core_dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_CORE_COORDINATE_DIMENSION_HPP
