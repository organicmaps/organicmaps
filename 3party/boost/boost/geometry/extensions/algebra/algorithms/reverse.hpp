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

#ifndef BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_REVERSE_HPP
#define BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_REVERSE_HPP

#include <boost/geometry/algorithms/reverse.hpp>

#include <boost/geometry/extensions/algebra/algorithms/detail.hpp>

namespace boost { namespace geometry
{

#ifndef DOXYGEN_NO_DISPATCH
namespace dispatch
{

// This is experimental implementation of reverse() which negates vectors
// and inverses rotations. It doesn't work for them as for Geometries.

template <typename Vector>
struct reverse<Vector, vector_tag>
{
    static inline void apply(Vector & v)
    {
        detail::algebra::neg<0, dimension<Vector>::value>(v);
    }
};

template <typename R>
struct reverse<R, rotation_quaternion_tag>
{
    static inline void apply(R & r)
    {
        detail::algebra::neg<1, 4>(r);
    }
};

template <typename R>
struct reverse<R, rotation_matrix_tag>
{
    static inline void apply(R & r)
    {
        detail::algebra::matrix_transpose<
            R, 0, 0, dimension<R>::value
        >::apply(r);
    }
};

} // namespace dispatch
#endif // DOXYGEN_NO_DISPATCH

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_EXTENSIONS_ALGEBRA_ALGORITHMS_CLEAR_HPP
